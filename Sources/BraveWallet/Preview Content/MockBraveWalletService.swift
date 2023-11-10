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
    BraveWallet.GoerliChainId: [.previewToken],
  ]
  private var defaultCurrency = CurrencyCode.usd
  private var defaultCryptocurrency = "eth"
  private var coin: BraveWallet.CoinType = .eth
  
  func ankrSupportedChainIds(_ completion: @escaping ([String]) -> Void) {
    completion([])
  }

  func userAssets(_ chainId: String, coin: BraveWallet.CoinType, completion: @escaping ([BraveWallet.BlockchainToken]) -> Void) {
    completion(assets[chainId] ?? [])
  }
  
  func allUserAssets(_ completion: @escaping ([BraveWallet.BlockchainToken]) -> Void) {
    let allAssets = assets.values.flatMap { $0 }
    completion(Array(allAssets))
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
  
  func add(_ observer: BraveWalletBraveWalletServiceTokenObserver) {
  }
  
  func setDefaultWallet(_ defaultEthWallet: BraveWallet.DefaultWallet) {
  }

  func notifySignMessageRequestProcessed(_ approved: Bool, id: Int32, signature: BraveWallet.ByteArrayStringUnion?, error: String?) {
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
  
  func notifyGetPublicKeyRequestProcessed(_ requestId: String, approved: Bool) {
  }
  
  func pendingGetEncryptionPublicKeyRequests() async -> [BraveWallet.GetEncryptionPublicKeyRequest] {
    return []
  }
  
  func notifyDecryptRequestProcessed(_ requestId: String, approved: Bool) {
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
  
  func addPermission(_ accountId: BraveWallet.AccountId, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func hasPermission(_ accounts: [BraveWallet.AccountId], completion: @escaping (Bool, [BraveWallet.AccountId]) -> Void) {
    completion(false, [])
  }
  
  func resetPermission(_ accountId: BraveWallet.AccountId, completion: @escaping (Bool) -> Void) {
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
  
  func notifySignTransactionRequestProcessed(_ approved: Bool, id: Int32, signature: BraveWallet.ByteArrayStringUnion?, error: String?) {
  }
  
  func notifySignAllTransactionsRequestProcessed(_ approved: Bool, id: Int32, signatures: [BraveWallet.ByteArrayStringUnion]?, error: String?) {
  }
  
  func base58Encode(_ addresses: [[NSNumber]], completion: @escaping ([String]) -> Void) {
    completion([])
  }

  func isPermissionDenied(_ coin: BraveWallet.CoinType, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func onOnboardingShown() {
  }
  
  func defaultEthereumWallet(_ completion: @escaping (BraveWallet.DefaultWallet) -> Void) {
    completion(.braveWallet)
  }
  
  func defaultSolanaWallet(_ completion: @escaping (BraveWallet.DefaultWallet) -> Void) {
    completion(.braveWallet)
  }
  
  func setDefaultEthereumWallet(_ defaultEthWallet: BraveWallet.DefaultWallet) {
  }
  
  func setDefaultSolanaWallet(_ defaultEthWallet: BraveWallet.DefaultWallet) {
  }
  
  func discoverAssetsOnAllSupportedChains() {
  }
  
  func nftDiscoveryEnabled(_ completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func setNftDiscoveryEnabled(_ enabled: Bool) {
  }
  
  func chainId(forActiveOrigin coin: BraveWallet.CoinType, completion: @escaping (String) -> Void) {
    completion(BraveWallet.MainnetChainId)
  }
  
  func setChainIdForActiveOrigin(_ coin: BraveWallet.CoinType, chainId: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func balanceScannerSupportedChains(_ completion: @escaping ([String]) -> Void) {
    completion([])
  }

  func discoverEthAllowances(_ completion: @escaping ([BraveWallet.AllowanceInfo]) -> Void) {
    completion([])
  }
  
  func setAssetSpamStatus(_ token: BraveWallet.BlockchainToken, status: Bool, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func simpleHashSpamNfTs(_ walletAddress: String, chainIds: [String], coin: BraveWallet.CoinType, cursor: String?, completion: @escaping ([BraveWallet.BlockchainToken], String?) -> Void) {
    completion([], nil)
  }
  
  func ensureSelectedAccount(forChain coin: BraveWallet.CoinType, chainId: String, completion: @escaping (BraveWallet.AccountId?) -> Void) {
    completion(nil)
  }
  
  func networkForSelectedAccount(onActiveOrigin completion: @escaping (BraveWallet.NetworkInfo?) -> Void) {
    completion(nil)
  }
  
  func setNetworkForSelectedAccountOnActiveOrigin(_ chainId: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func convertFevm(toFvmAddress isMainnet: Bool, fevmAddresses: [String], completion: @escaping ([String: String]) -> Void) {
    completion([:])
  }

  func pendingSignMessageErrors(_ completion: @escaping ([BraveWallet.SignMessageError]) -> Void) {
    completion([])
  }
  
  func notifySignMessageErrorProcessed(_ errorId: String) {

  }

  func generateReceiveAddress(_ accountId: BraveWallet.AccountId, completion: @escaping (String?, String?) -> Void) {
    completion(nil, "Error Message")
  }
}
#endif
