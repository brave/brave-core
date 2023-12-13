// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared
import BraveWallet

extension Web3Service {
  
  var errorTitle: String {
    switch self {
    case .solana:
      return Strings.Wallet.snsDomainInterstitialPageTitle
    case .ethereum:
      return Strings.Wallet.ensDomainInterstitialPageTitle
    case .ethereumOffchain:
      return Strings.Wallet.ensOffchainDomainInterstitialPageTitle
    case .unstoppable:
      return Strings.Wallet.udDomainInterstitialPageTitle
    }
  }
  
  var errorDescription: String {
    switch self {
    case .solana:
      let braveWikiUrl = WalletConstants.snsBraveWikiURL.absoluteString
      return String.localizedStringWithFormat(
        Strings.Wallet.snsDomainInterstitialPageDescription,
        braveWikiUrl)
    case .ethereum:
      let termsOfUseUrl = WalletConstants.ensTermsOfUseURL.absoluteString
      let privacyPolicyUrl = WalletConstants.ensPrivacyPolicyURL.absoluteString
      return String.localizedStringWithFormat(
        Strings.Wallet.ensDomainInterstitialPageDescription,
        termsOfUseUrl,
        Strings.Wallet.web3DomainInterstitialPageTAndU,
        privacyPolicyUrl,
        Strings.Wallet.web3DomainInterstitialPagePrivacyPolicy)
    case .ethereumOffchain:
      let learnMore = WalletConstants.braveWalletENSOffchainURL.absoluteString
      return String.localizedStringWithFormat(
        Strings.Wallet.ensOffchainDomainInterstitialPageDescription,
        learnMore,
        Strings.Wallet.learnMoreButton)
    case .unstoppable:
      let termsOfUseUrl = WalletConstants.ensTermsOfUseURL.absoluteString
      let privacyPolicyUrl = WalletConstants.ensPrivacyPolicyURL.absoluteString
      let nonCryptoExtensions = WalletConstants.supportedUDExtensions.filter { $0 != ".crypto" }
      return String.localizedStringWithFormat(
        Strings.Wallet.udDomainInterstitialPageDescription,
        nonCryptoExtensions.joined(separator: ", "),
        termsOfUseUrl,
        Strings.Wallet.web3DomainInterstitialPageTAndU,
        privacyPolicyUrl,
        Strings.Wallet.web3DomainInterstitialPagePrivacyPolicy)
    }
  }
  
  var disableButtonTitle: String {
    Strings.Wallet.web3DomainInterstitialPageButtonDisable
  }
  
  var proceedButtonTitle: String {
    switch self {
    case .solana:
      return Strings.Wallet.snsDomainInterstitialPageButtonProceed
    case .ethereum:
      return Strings.Wallet.ensDomainInterstitialPageButtonProceed
    case .ethereumOffchain:
      return Strings.Wallet.ensOffchainDomainInterstitialPageButtonProceed
    case .unstoppable:
      return Strings.Wallet.ensDomainInterstitialPageButtonProceed
    }
  }
}

public class Web3DomainHandler: InternalSchemeResponse {
  
  public static let path = "web3/ddns"

  public init() {}
  
  public func response(forRequest request: URLRequest) -> (URLResponse, Data)? {
    guard let url = request.url else { return nil }
    let response = InternalSchemeHandler.response(forUrl: url)
    guard let path = Bundle.module.path(forResource: "Web3Domain", ofType: "html"),
          let serviceId = request.url?.getQuery()[Web3NameServiceScriptHandler.ParamKey.serviceId.rawValue],
          let service = Web3Service(rawValue: serviceId) else {
      return nil
    }
    
    guard var html = try? String(contentsOfFile: path) else {
      assert(false)
      return nil
    }
    
    let variables = [
      "page_title": request.url?.displayURL?.absoluteDisplayString ?? "",
      "error_title": service.errorTitle,
      "error_description": service.errorDescription,
      "button_disable": service.disableButtonTitle,
      "button_procced": service.proceedButtonTitle,
      "message_handler": Web3NameServiceScriptHandler.messageHandlerName,
      Web3NameServiceScriptHandler.ParamKey.serviceId.rawValue: service.id
    ]
    
    variables.forEach { (arg, value) in
      html = html.replacingOccurrences(of: "%\(arg)%", with: value)
    }
    
    guard let data = html.data(using: .utf8) else {
      return nil
    }
    
    return (response, data)
  }
}
