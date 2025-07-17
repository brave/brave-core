// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import Foundation
import Web

extension BraveShieldsTabHelper: TabPolicyDecider {
  @MainActor public func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard let requestURL = request.url,
      ["http", "https", "data", "blob", "file"].contains(requestURL.scheme)
    else {
      return .allow
    }
    // Only use main document URL, not the request URL
    // If an iFrame is loaded, shields depending on the main frame, not the iFrame request

    // Weird behavior here with `targetFram` and `sourceFrame`, on refreshing page `sourceFrame` is not nil (it is non-optional)
    //  however, it is still an uninitialized object, making it an unreliable source to compare `isMainFrame` against.
    // Rather than using `sourceFrame.isMainFrame` or even comparing `sourceFrame == targetFrame`, a simple URL check is used.
    // No adblocking logic is be used on session restore urls. It uses javascript to retrieve the
    // request then the page is reloaded with a proper url and adblocking rules are applied.
    guard let mainDocumentURL = request.mainDocumentURL,
      mainDocumentURL.schemelessAbsoluteString == requestURL.schemelessAbsoluteString,
      requestInfo.isMainFrame
    else { return .allow }
    // Identify specific block lists that need to be applied to the requesting domain
    let isBraveShieldsEnabled =
      tab.braveShieldsHelper?.isBraveShieldsEnabled(
        for: mainDocumentURL,
        isPrivate: tab.isPrivate
      ) ?? true
    let shieldLevel =
      tab.braveShieldsHelper?.shieldLevel(
        for: mainDocumentURL,
        isPrivate: tab.isPrivate,
        considerAllShieldsOption: true
      ) ?? .standard

    // Load rule lists
    let ruleLists = await AdBlockGroupsManager.shared.ruleLists(
      isBraveShieldsEnabled: isBraveShieldsEnabled,
      shieldLevel: shieldLevel
    )
    tab.contentBlocker?.set(ruleLists: ruleLists)
    return .allow
  }
}
