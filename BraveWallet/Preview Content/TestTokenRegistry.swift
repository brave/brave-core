/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveCore

/// A test token registry which can be passed to a ``TokenRegistryStore`` that implements some basic
/// functionality for the use of SwiftUI Previews.
///
/// - note: Do not use this directly, use ``TokenRegistryStore.previewStore``
class TestTokenRegistry: BraveWalletERCTokenRegistry {
  static let testTokens: [BraveWallet.ERCToken] = [
    .init(contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef", name: "Basic Attention Token", logo: "", isErc20: true, isErc721: false, symbol: "BAT", decimals: 18, visible: true, tokenId: ""),
    .init(contractAddress: "0xB8c77482e45F1F44dE1745F52C74426C631bDD52", name: "BNB", logo: "", isErc20: true, isErc721: false, symbol: "BNB", decimals: 18, visible: true, tokenId: ""),
    .init(contractAddress: "0xdac17f958d2ee523a2206206994597c13d831ec7", name: "Tether USD", logo: "", isErc20: true, isErc721: false, symbol: "USDT", decimals: 6, visible: true, tokenId: ""),
    .init(contractAddress: "0x57f1887a8bf19b14fc0df6fd9b2acc9af147ea85", name: "Ethereum Name Service", logo: "", isErc20: false, isErc721: true, symbol: "ENS", decimals: 1, visible: true, tokenId: "")
  ]
  
  func token(byContract contract: String, completion: @escaping (BraveWallet.ERCToken?) -> Void) {
    if let token = Self.testTokens.first(where: { $0.contractAddress == contract }) {
      completion(token)
      return
    }
    completion(.init())
  }
  
  func token(bySymbol symbol: String, completion: @escaping (BraveWallet.ERCToken?) -> Void) {
    if let token = Self.testTokens.first(where: { $0.symbol == symbol }) {
      completion(token)
      return
    }
    completion(.init())
  }
  
  func allTokens(_ completion: @escaping ([BraveWallet.ERCToken]) -> Void) {
    completion(Self.testTokens)
  }
  
  func buyTokens(_ completion: @escaping ([BraveWallet.ERCToken]) -> Void) {
    completion(Self.testTokens)
  }
  
  func buyUrl(_ address: String, symbol: String, amount: String, completion: @escaping (String) -> Void) {
    
  }
}
