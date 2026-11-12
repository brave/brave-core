// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import UserAgent
import Web
import WebKit

extension TabDataValues {
  private struct TorPageToolsKey: TabDataKey {
    static var defaultValue: TorPageToolsTabHelper?
  }
  var torPageTools: TorPageToolsTabHelper? {
    get { self[TorPageToolsKey.self] }
    set { self[TorPageToolsKey.self] = newValue }
  }
}

@MainActor
final class TorPageToolsTabHelper: TabObserver {
  private weak var tab: TorTabState?
  private var navigationGeneration = 0
  private var readabilityTask: Task<Void, Never>?
  private(set) var readerAvailableOrActive = false

  private static let script: String? = {
    guard let url = Bundle.module.url(forResource: "TorPageTools", withExtension: "js")
    else { return nil }
    return try? String(contentsOf: url, encoding: .utf8)
  }()

  init(tab: TorTabState) {
    self.tab = tab
    tab.userAgentProvider = { type in
      type == .desktop ? UserAgent.desktopMasked : UserAgent.mobileMasked
    }
    tab.addObserver(self)
  }

  deinit { readabilityTask?.cancel() }

  func tabDidCreateWebView(_ tab: some TabState) {
    guard let script = Self.script else { return }
    tab.configuration?.userContentController.addUserScript(
      WKUserScript(source: script, injectionTime: .atDocumentStart,
                   forMainFrameOnly: true, in: .defaultClient)
    )
  }

  func tabDidStartNavigation(_ tab: some TabState) {
    navigationGeneration += 1
    readabilityTask?.cancel()
    readerAvailableOrActive = false
  }

  func tabDidFinishNavigation(_ tab: some TabState) {
    let generation = navigationGeneration
    readabilityTask = Task { [weak self, weak tab] in
      guard let tab else { return }
      let result = try? await tab.evaluateJavaScript(
        functionName: "globalThis.__braveTorPageTools.isReaderAvailable",
        contentWorld: .defaultClient
      )
      guard let self, !Task.isCancelled, generation == navigationGeneration else { return }
      readerAvailableOrActive = (result as? Bool) == true
      let allowsNightMode = tab.visibleURL.map { !NightModeTabHelper.isNightModeBlockedURL($0) } ?? false
      setNightMode(Preferences.General.nightModeEnabled.value && allowsNightMode)
    }
  }

  func toggleReaderMode() {
    guard let tab else { return }
    let generation = navigationGeneration
    Task { [weak self, weak tab] in
      guard let tab else { return }
      _ = try? await tab.evaluateJavaScript(
        functionName: "globalThis.__braveTorPageTools.toggleReader",
        contentWorld: .defaultClient
      )
      guard let self, generation == navigationGeneration else { return }
      tab.refreshPreview()
    }
  }

  func setNightMode(_ enabled: Bool) {
    tab?.evaluateJavaScript(
      functionName: "globalThis.__braveTorPageTools.setNightMode", args: [enabled],
      contentWorld: .defaultClient
    )
  }
}
