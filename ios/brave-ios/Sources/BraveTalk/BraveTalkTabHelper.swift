// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
@_spi(ChromiumWebViewAccess) import Web

extension TabDataValues {
  private struct BraveTalkTabHelperKey: TabDataKey {
    static var defaultValue: BraveTalkTabHelper?
  }
  public var braveTalk: BraveTalkTabHelper? {
    get { self[BraveTalkTabHelperKey.self] }
    set { self[BraveTalkTabHelperKey.self] = newValue }
  }
}

@MainActor
public class BraveTalkTabHelper: TabObserver, @preconcurrency BraveTalkTabHelperBridge {
  private weak var tab: (any TabState)?
  private let coordinator: BraveTalkJitsiCoordinator
  public var onExitCall: (() -> Void)?

  public init(tab: some TabState, coordinator: BraveTalkJitsiCoordinator) {
    self.tab = tab
    self.coordinator = coordinator

    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  public func tabDidCreateWebView(_ tab: some TabState) {
    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      BraveWebView.from(tab: tab)?.setBraveTalkHelper(self)
    }
  }

  public func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  public func launchBraveTalk(withRoom room: String, jwtKey: String) {
    guard let tab, let host = tab.lastCommittedURL?.host else { return }
    coordinator.launchNativeBraveTalk(
      for: room,
      token: jwtKey,
      host: host,
      onEnterCall: { [weak tab] in
        tab?.stopLoading()
      },
      onExitCall: { [weak self] in
        self?.onExitCall?()
      }
    )
  }
}
