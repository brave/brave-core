// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveShields
import Foundation
import Shared
import WebKit

public class HTTPBlockedHandler: InternalSchemeResponse {
  public static let path = InternalURL.Path.httpBlocked.rawValue

  public init() {}

  public func response(forRequest request: URLRequest) async -> (URLResponse, Data)? {
    guard let url = request.url, let internalURL = InternalURL(url),
      let originalURL = internalURL.extractedUrlParam
    else { return nil }
    let response = InternalSchemeHandler.response(forUrl: internalURL.url)

    guard let asset = Bundle.module.url(forResource: "HTTPBlocked", withExtension: "html") else {
      assert(false)
      return nil
    }

    guard var html = await AsyncFileManager.default.utf8Contents(at: asset) else {
      assert(false)
      return nil
    }

    html =
      html
      .replacingOccurrences(
        of: "%page_title%",
        with: Strings.Shields.siteIsNotSecure
      )
      .replacingOccurrences(
        of: "%blocked_title%",
        with: String.localizedStringWithFormat(
          Strings.Shields.theConnectionIsNotSecure,
          "<tt>\(originalURL.host ?? originalURL.domainURL.absoluteDisplayString)</tt>"
        )
      )
      .replacingOccurrences(
        of: "%blocked_description%",
        with: Strings.Shields.httpBlockedDescription
      )
      .replacingOccurrences(
        of: "%learn_more%",
        with: Strings.learnMore
      )
      .replacingOccurrences(
        of: "%proceed_action%",
        with: Strings.Shields.domainBlockedProceedAction
      )
      .replacingOccurrences(of: "%go_back_action%", with: Strings.Shields.domainBlockedGoBackAction)
      .replacingOccurrences(
        of: "%message_handler%",
        with: HTTPBlockedScriptHandler.messageHandlerName
      )
      .replacingOccurrences(of: "%security_token%", with: UserScriptManager.securityToken)

    html = html.replacingOccurrences(
      of: "<html lang=\"en\">",
      with: "<html lang=\"\(Locale.current.language.minimalIdentifier)\">"
    )

    let data = Data(html.utf8)
    return (response, data)
  }
}
