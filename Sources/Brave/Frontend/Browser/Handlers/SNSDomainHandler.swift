// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared

public class SNSDomainHandler: InternalSchemeResponse {
  public static let path = "web3/sns"
  
  /// The term of use page of Syndica server to resolve SNS domain
  let snsThirdPartyTermsofUseLink: URL = URL(string: "https://syndica.io/terms-and-conditions/")!
  
  /// The privacy policy page of Syndica server to resolve SNS domain
  let snsThirdPartyPrivacyPolicyLink: URL = URL(string: "https://syndica.io/privacy-policy/")!
  
  public func response(forRequest request: URLRequest) -> (URLResponse, Data)? {
    guard let url = request.url else { return nil }
    let response = InternalSchemeHandler.response(forUrl: url)
    guard let path = Bundle.module.path(forResource: "SNSDomain", ofType: "html")
    else {
      return nil
    }
    
    guard var html = try? String(contentsOfFile: path) else {
      assert(false)
      return nil
    }
    
    let variables = [
      "page_title": request.url?.displayURL?.absoluteDisplayString ?? "",
      "error_title": Strings.snsDomainInterstitialPageTitle,
      "error_description": String.localizedStringWithFormat(Strings.snsDomainInterstitialPageDescription, snsThirdPartyTermsofUseLink.absoluteString, Strings.snsDomainInterstitialPageTAndU, snsThirdPartyPrivacyPolicyLink.absoluteString, Strings.snsDomainInterstitialPagePrivacyPolicy),
      "button_disable": Strings.snsDomainInterstitialPageButtonDisable,
      "button_procced": Strings.snsDomainInterstitialPageButtonProceed,
      "message_handler": Web3NameServiceScriptHandler.messageHandlerName,
    ]
    
    variables.forEach { (arg, value) in
      html = html.replacingOccurrences(of: "%\(arg)%", with: value)
    }
    
    guard let data = html.data(using: .utf8) else {
      return nil
    }
    
    return (response, data)
  }
  
  public init() { }
}
