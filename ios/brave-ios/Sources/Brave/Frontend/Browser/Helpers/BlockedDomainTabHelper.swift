// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Shared
import Web

extension TabDataValues {
  private struct BlockedDomainTabHelperKey: TabDataKey {
    static var defaultValue: BlockedDomainTabHelper?
  }
  public var blockedDomainTabHelper: BlockedDomainTabHelper? {
    get { self[BlockedDomainTabHelperKey.self] }
    set { self[BlockedDomainTabHelperKey.self] = newValue }
  }
}

@MainActor
public class BlockedDomainTabHelper: TabPolicyDecider {

  weak var tab: (any TabState)?

  init(tab: some TabState) {
    self.tab = tab
    tab.addPolicyDecider(self)
  }

  /// Whether shields would block this request as a blocked-domain navigation.
  static func isDomainBlocked(_ requestURL: URL, tab: some TabState) async -> Bool {
    guard let etldP1 = requestURL.baseDomain,
      tab.proceedAnywaysDomainList?.contains(etldP1) == false
    else { return false }

    let shieldLevel =
      tab.braveShieldsHelper?.shieldLevel(
        for: requestURL,
        considerAllShieldsOption: true
      ) ?? .standard

    return await AdBlockGroupsManager.shared.shouldBlock(
      requestURL: requestURL,
      sourceURL: requestURL,
      resourceType: .document,
      isAdBlockEnabled: shieldLevel.isEnabled,
      isAdBlockModeAggressive: shieldLevel.isAggressive
    )
  }

  public func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard let requestURL = request.url
    else { return .cancel }

    if ["http", "https", "data", "blob", "file"].contains(requestURL.scheme),
      requestInfo.isMainFrame,
      await Self.isDomainBlocked(requestURL, tab: tab),
      let internalURL = requestURL.encodeEmbeddedInternalURL(for: .blocked)
    {
      tab.loadRequest(PrivilegedRequest(url: internalURL) as URLRequest)
      return .cancel
    }

    return .allow
  }

}
