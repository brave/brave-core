// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Preferences
@_spi(ChromiumWebViewAccess) import Web

extension TabDataValues {
  private struct RequestBlockingTabHelperKey: TabDataKey {
    static var defaultValue: RequestBlockingTabHelper?
  }
  public var requestBlockingTabHelper: RequestBlockingTabHelper? {
    get { self[RequestBlockingTabHelperKey.self] }
    set { self[RequestBlockingTabHelperKey.self] = newValue }
  }
}

public class RequestBlockingTabHelper: TabObserver {

  private weak var tab: (any TabState)?

  public init(
    tab: some TabState
  ) {
    self.tab = tab
    tab.addObserver(self)
  }

  // MARK: - TabObserver

  public func tabDidCreateWebView(_ tab: some TabState) {
    BraveWebView.from(tab: tab)?.setRequestBlockingTabHelperBridge(self)
  }

  public func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}

@MainActor extension RequestBlockingTabHelper: RequestBlockingTabHelperBridge {

  public func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: String) async -> Bool {
    guard let tab,
      let resourceType = AdblockEngine.ResourceType(rawValue: resourceType),
      let adblockMode = tab.braveShieldsHelper?.shieldLevel(
        for: sourceURL,
        considerAllShieldsOption: true
      ).adBlockMode
    else {
      return false
    }
    return await AdBlockGroupsManager.shared.shouldBlock(
      requestURL: requestURL,
      sourceURL: sourceURL,
      resourceType: resourceType,
      isAdBlockEnabled: adblockMode.shieldLevel.isEnabled,
      isAdBlockModeAggressive: adblockMode.shieldLevel.isAggressive
    )
  }
}
