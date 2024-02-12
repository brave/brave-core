// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared
import BraveWallet

public class IPFSSchemeHandler: InternalSchemeResponse {
  public static let path = "web3/ipfs"
  
  public func response(forRequest request: URLRequest) -> (URLResponse, Data)? {
    guard let url = request.url else { return nil }
    let response = InternalSchemeHandler.response(forUrl: url)
    guard let path = Bundle.module.path(forResource: "IPFSPreference", ofType: "html")
    else {
      return nil
    }
    
    guard var html = try? String(contentsOfFile: path) else {
      assert(false)
      return nil
    }
    
    let variables = [
      "page_title": request.url?.displayURL?.absoluteDisplayString ?? "",
      "interstitial_ipfs_title": Strings.Wallet.web3IPFSInterstitialIPFSTitle,
      "interstitial_ipfs_privacy": String.localizedStringWithFormat(Strings.Wallet.web3IPFSInterstitialIPFSPrivacy, WalletConstants.ipfsLearnMoreLink.absoluteString, Strings.learnMore.lowercased().capitalizeFirstLetter),
      "interstitial_ipfs_public_gateway": Strings.Wallet.web3IPFSInterstitialIPFSPublicGateway,
      "button_disable": Strings.Wallet.web3DomainInterstitialPageButtonDisable,
      "button_procced": Strings.Wallet.web3IPFSInterstitialProceedButton,
      "message_handler": Web3IPFSScriptHandler.messageHandlerName,
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
