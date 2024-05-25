// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveShields
import Foundation
import Shared
import WebKit

public class BlockedDomainHandler: InternalSchemeResponse {
  public static let path = InternalURL.Path.blocked.rawValue

  public init() {}

  public func response(forRequest request: URLRequest) -> (URLResponse, Data)? {
    guard let url = request.url, let internalURL = InternalURL(url),
      let originalURL = internalURL.extractedUrlParam
    else { return nil }
    let response = InternalSchemeHandler.response(forUrl: internalURL.url)

    guard let asset = Bundle.module.path(forResource: "BlockedDomain", ofType: "html") else {
      assert(false)
      return nil
    }

    var html = try? String(contentsOfFile: asset)
      .replacingOccurrences(of: "%page_title%", with: Strings.Shields.domainBlockedTitle)
      .replacingOccurrences(of: "%blocked_title%", with: Strings.Shields.domainBlockedPageTitle)
      .replacingOccurrences(
        of: "%blocked_subtitle%",
        with: Strings.Shields.domainBlockedPageMessage
      )
      .replacingOccurrences(
        of: "%blocked_domain%",
        with: originalURL.domainURL.absoluteDisplayString
      )
      .replacingOccurrences(
        of: "%blocked_description%",
        with: Strings.Shields.domainBlockedPageDescription
      )
      .replacingOccurrences(
        of: "%proceed_action%",
        with: Strings.Shields.domainBlockedProceedAction
      )
      .replacingOccurrences(of: "%go_back_action%", with: Strings.Shields.domainBlockedGoBackAction)
      .replacingOccurrences(
        of: "%message_handler%",
        with: BlockedDomainScriptHandler.messageHandlerName
      )
      .replacingOccurrences(of: "%security_token%", with: UserScriptManager.securityToken)

    html = html?.replacingOccurrences(
      of: "<html lang=\"en\">",
      with: "<html lang=\"\(Locale.current.language.minimalIdentifier)\">"
    )

    guard let data = html?.data(using: .utf8) else {
      return nil
    }

    return (response, data)
  }
}
