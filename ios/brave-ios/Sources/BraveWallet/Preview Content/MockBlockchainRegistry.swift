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
class MockBlockchainRegistry: BraveWalletBlockchainRegistry {
  static let testTokens: [BraveWallet.BlockchainToken] = [
    .init(contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef", name: "Basic Attention Token", logo: "", isErc20: true, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "BAT", decimals: 18, visible: true, tokenId: "", coingeckoId: "", chainId: "", coin: .eth),
    .init(contractAddress: "0xB8c77482e45F1F44dE1745F52C74426C631bDD52", name: "BNB", logo: "", isErc20: true, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "BNB", decimals: 18, visible: true, tokenId: "", coingeckoId: "", chainId: "", coin: .eth),
    .init(contractAddress: "0xdac17f958d2ee523a2206206994597c13d831ec7", name: "Tether USD", logo: "", isErc20: true, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "USDT", decimals: 6, visible: true, tokenId: "", coingeckoId: "", chainId: "", coin: .eth),
    .init(contractAddress: "0x57f1887a8bf19b14fc0df6fd9b2acc9af147ea85", name: "Ethereum Name Service", logo: "", isErc20: false, isErc721: true, isErc1155: false, isNft: false, isSpam: false, symbol: "ENS", decimals: 1, visible: true, tokenId: "", coingeckoId: "", chainId: "", coin: .eth),
    .init(contractAddress: "0xad6d458402f60fd3bd25163575031acdce07538d", name: "DAI Stablecoin", logo: "", isErc20: true, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "DAI", decimals: 18, visible: false, tokenId: "", coingeckoId: "", chainId: "", coin: .eth),
    .init(contractAddress: "0x7D1AfA7B718fb893dB30A3aBc0Cfc608AaCfeBB0", name: "MATIC", logo: "", isErc20: true, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "MATIC", decimals: 18, visible: true, tokenId: "", coingeckoId: "", chainId: "", coin: .eth)
  ]

  func token(byAddress chainId: String, coin: BraveWallet.CoinType, address: String, completion: @escaping (BraveWallet.BlockchainToken?) -> Void) {
    if let token = Self.testTokens.first(where: { $0.contractAddress == address }) {
      completion(token)
      return
    }
    completion(.init())
  }

  func token(bySymbol chainId: String, coin: BraveWallet.CoinType, symbol: String, completion: @escaping (BraveWallet.BlockchainToken?) -> Void) {
    if let token = Self.testTokens.first(where: { $0.symbol == symbol }) {
      completion(token)
      return
    }
    completion(.init())
  }

  func allTokens(_ chainId: String, coin: BraveWallet.CoinType, completion: @escaping ([BraveWallet.BlockchainToken]) -> Void) {
    completion(Self.testTokens)
  }

  func buyTokens(_ provider: BraveWallet.OnRampProvider, chainId: String, completion: @escaping ([BraveWallet.BlockchainToken]) -> Void) {
    completion(Self.testTokens)
  }
  
  func sellTokens(_ provider: BraveWallet.OffRampProvider, chainId: String, completion: @escaping ([BraveWallet.BlockchainToken]) -> Void) {
    completion(Self.testTokens)
  }
  
  func providersBuyTokens(_ providers: [NSNumber], chainId: String, completion: @escaping ([BraveWallet.BlockchainToken]) -> Void) {
    completion(Self.testTokens)
  }

  func buyUrl(_ provider: BraveWallet.OnRampProvider, chainId: String, address: String, symbol: String, amount: String, completion: @escaping (String, String?) -> Void) {
    completion("", nil)
  }
  
  func searchNetworks(_ chainIdFilter: String?, chainNameFilter: String?, completion: @escaping ([BraveWallet.NetworkInfo]) -> Void) {
    completion([])
  }
  
  func prepopulatedNetworks(_ completion: @escaping ([BraveWallet.NetworkInfo]) -> Void) {
    completion([])
  }
  
  func onRampCurrencies(_ completion: @escaping ([BraveWallet.OnRampCurrency]) -> Void) {
    completion([])
  }
  
  func topDapps(_ chainId: String, coin: BraveWallet.CoinType, completion: @escaping ([BraveWallet.Dapp]) -> Void) {
    completion([])
  }
  
  func coingeckoId(_ chainId: String, contractAddress: String, completion: @escaping (String?) -> Void) {
    completion(nil)
  }
}
