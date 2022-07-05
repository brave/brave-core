// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

#if DEBUG
/// A test wallet service that implements some basic functionality for the use of SwiftUI Previews.
///
/// - note: Do not use this directly, use ``NetworkStore.previewStore``
class MockBraveWalletService: BraveWalletBraveWalletService {
  private var assets: [String: [BraveWallet.BlockchainToken]] = [
    BraveWallet.MainnetChainId: [.previewToken],
    BraveWallet.RopstenChainId: [.previewToken],
  ]
  private var defaultCurrency = CurrencyCode.usd
  private var defaultCryptocurrency = "eth"
  private var coin: BraveWallet.CoinType = .eth

  func userAssets(_ chainId: String, coin: BraveWallet.CoinType, completion: @escaping ([BraveWallet.BlockchainToken]) -> Void) {
    completion(assets[chainId] ?? [])
  }

  func addUserAsset(_ token: BraveWallet.BlockchainToken, completion: @escaping (Bool) -> Void) {
    assets[token.chainId]?.append(token)
  }

  func removeUserAsset(_ token: BraveWallet.BlockchainToken, completion: @escaping (Bool) -> Void) {
    assets[token.chainId]?.removeAll(where: { $0.contractAddress == token.contractAddress })
  }

  func setUserAssetVisible(_ token: BraveWallet.BlockchainToken, visible: Bool, completion: @escaping (Bool) -> Void) {
    let chainAssets = assets[token.chainId]
    if let index = chainAssets?.firstIndex(where: { $0.contractAddress == token.contractAddress }) {
      chainAssets?[index].visible = visible
    }
  }

  func `import`(from type: BraveWallet.ExternalWalletType, password: String, newPassword: String, completion: @escaping (Bool, String?) -> Void) {
    completion(false, nil)
  }

  func defaultWallet(_ completion: @escaping (BraveWallet.DefaultWallet) -> Void) {
    completion(.braveWallet)
  }

  func hasEthereumPermission(_ origin: URLOrigin, account: String, completion: @escaping (Bool, Bool) -> Void) {
    completion(false, false)
  }

  func resetEthereumPermission(_ origin: URLOrigin, account: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func activeOrigin(_ completion: @escaping (BraveWallet.OriginInfo) -> Void) {
    completion(.init())
  }

  func pendingSignMessageRequests(_ completion: @escaping ([BraveWallet.SignMessageRequest]) -> Void) {
    completion([])
  }

  func pendingAddSuggestTokenRequests(_ completion: @escaping ([BraveWallet.AddSuggestTokenRequest]) -> Void) {
    completion([])
  }

  func defaultBaseCurrency(_ completion: @escaping (String) -> Void) {
    completion(defaultCurrency.code)
  }

  func setDefaultBaseCurrency(_ currency: String) {
    defaultCurrency = CurrencyCode(code: currency)
  }

  func defaultBaseCryptocurrency(_ completion: @escaping (String) -> Void) {
    completion(defaultCryptocurrency)
  }

  func setDefaultBaseCryptocurrency(_ cryptocurrency: String) {
    defaultCryptocurrency = cryptocurrency
  }

  func isExternalWalletInstalled(_ type: BraveWallet.ExternalWalletType, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func isExternalWalletInitialized(_ type: BraveWallet.ExternalWalletType, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func addEthereumPermission(_ origin: URLOrigin, account: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func add(_ observer: BraveWalletBraveWalletServiceObserver) {
  }

  func setDefaultWallet(_ defaultWallet: BraveWallet.DefaultWallet) {
  }

  func notifySignMessageRequestProcessed(_ approved: Bool, id: Int32) {
  }

  func notifySignMessageHardwareRequestProcessed(_ approved: Bool, id: Int32, signature: String, error: String) {
  }

  func notifyAddSuggestTokenRequestsProcessed(_ approved: Bool, contractAddresses: [String]) {
  }

  func reset() {
  }
  
  func activeOrigin(_ completion: @escaping (String, String) -> Void) {
    completion("", "")
  }
  
  func notifyGetPublicKeyRequestProcessed(_ approved: Bool, origin: URLOrigin) {
  }
  
  func pendingGetEncryptionPublicKeyRequests() async -> [BraveWallet.GetEncryptionPublicKeyRequest] {
    return []
  }
  
  func notifyDecryptRequestProcessed(_ approved: Bool, origin: URLOrigin) {
  }
  
  func pendingDecryptRequests() async -> [BraveWallet.DecryptRequest] {
    return []
  }
  
  func showWalletTestNetworks(_ completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func selectedCoin(_ completion: @escaping (BraveWallet.CoinType) -> Void) {
    completion(coin)
  }
  
  func setSelectedCoin(_ coin: BraveWallet.CoinType) {
    self.coin = coin
  }
  
  func addPermission(_ coin: BraveWallet.CoinType, origin: URLOrigin, account: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func hasPermission(_ coin: BraveWallet.CoinType, origin: URLOrigin, account: String, completion: @escaping (Bool, Bool) -> Void) {
    completion(false, false)
  }
  
  func resetPermission(_ coin: BraveWallet.CoinType, origin: URLOrigin, account: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func isBase58EncodedSolanaPubkey(_ key: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func eTldPlusOne(fromOrigin origin: URLOrigin, completion: @escaping (BraveWallet.OriginInfo) -> Void) {
    completion(.init())
  }
  
  func webSites(withPermission coin: BraveWallet.CoinType, completion: @escaping ([String]) -> Void) {
    completion([])
  }
  
  func resetWebSitePermission(_ coin: BraveWallet.CoinType, formedWebsite: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func pendingSignTransactionRequests(_ completion: @escaping ([BraveWallet.SignTransactionRequest]) -> Void) {
    completion([])
  }
  
  func pendingSignAllTransactionsRequests(_ completion: @escaping ([BraveWallet.SignAllTransactionsRequest]) -> Void) {
    completion([])
  }
  
  func notifySignTransactionRequestProcessed(_ approved: Bool, id: Int32) {
  }
  
  func notifySignAllTransactionsRequestProcessed(_ approved: Bool, id: Int32) {
  }
}
#endif
