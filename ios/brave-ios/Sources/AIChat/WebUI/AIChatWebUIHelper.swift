// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
@_spi(ChromiumWebViewAccess) import Web

/// A tab helper for interacting with Leo AI Chat running in WebUI
public class AIChatWebUIHelper: NSObject, TabObserver, AIChatUIHandler {
  private(set) weak var tab: (any TabState)?

  public init?(tab: some TabState) {
    if !tab.isChromiumTab || !FeatureList.kAIChatWebUIEnabled.enabled {
      return nil
    }
    self.tab = tab
    super.init()
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  // MARK: - TabObserver

  public func tabDidCreateWebView(_ tab: some TabState) {
    // Update the UI handler if it was set prior to web view creation
    BraveWebView.from(tab: tab)?.aiChatUIHandler = self
  }

  public func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: - AIChatUIHandler

  public func handleVoiceRecognitionRequest(_ completion: @escaping (String?) -> Void) {
    completion(nil)
  }

  public func openAIChatSettings() {
  }

  public func goPremium() {
  }

  public func managePremium() {
  }

  public func closeUI() {
  }

  public func open(_ url: URL) {
  }
}

extension TabDataValues {
  private struct AIChatWebUIHelperKey: TabDataKey {
    static var defaultValue: AIChatWebUIHelper? = nil
  }

  public var aiChatWebUIHelper: AIChatWebUIHelper? {
    get { self[AIChatWebUIHelperKey.self] }
    set { self[AIChatWebUIHelperKey.self] = newValue }
  }
}
