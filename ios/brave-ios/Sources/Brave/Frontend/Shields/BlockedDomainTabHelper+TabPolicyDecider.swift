// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Shared
import Web

extension BlockedDomainTabHelper: TabPolicyDecider {
  public func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard let requestURL = request.url,
      ["http", "https", "data", "blob", "file"].contains(requestURL.scheme)
    else {
      return .cancel
    }
    if requestInfo.isMainFrame,
      let etldP1 = requestURL.baseDomain,
      tab.proceedAnywaysDomainList?.contains(etldP1) == false
    {
      let shieldLevel =
        tab.braveShieldsHelper?.shieldLevel(
          for: requestURL,
          considerAllShieldsOption: true
        ) ?? .standard

      let shouldBlock = await AdBlockGroupsManager.shared.shouldBlock(
        requestURL: requestURL,
        sourceURL: requestURL,
        resourceType: .document,
        isAdBlockEnabled: shieldLevel.isEnabled,
        isAdBlockModeAggressive: shieldLevel.isAggressive
      )

      if shouldBlock, let url = requestURL.encodeEmbeddedInternalURL(for: .blocked) {
        let request = PrivilegedRequest(url: url) as URLRequest
        tab.loadRequest(request)
        return .cancel
      }
    }
    return .allow
  }
}
