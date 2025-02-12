// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

#if DEBUG
/// A test wallet service that implements some basic functionality for the use of SwiftUI Previews.
///
/// - note: Do not use this directly, use ``NetworkStore.previewStore``
class MockBraveWalletService: BraveWalletBraveWalletService {
  private var assets: [String: [BraveWallet.BlockchainToken]] = [
    BraveWallet.MainnetChainId: [.previewToken],
    BraveWallet.SepoliaChainId: [.previewToken],
  ]
  private var defaultCurrency = CurrencyCode.usd
  private var defaultCryptocurrency = "eth"
  private var coin: BraveWallet.CoinType = .eth

  func ankrSupportedChainIds(completion: @escaping ([String]) -> Void) {
    completion([])
  }

  func userAssets(
    chainId: String,
    coin: BraveWallet.CoinType,
    completion: @escaping ([BraveWallet.BlockchainToken]) -> Void
  ) {
    completion(assets[chainId] ?? [])
  }

  func allUserAssets(completion: @escaping ([BraveWallet.BlockchainToken]) -> Void) {
    let allAssets = assets.values.flatMap { $0 }
    completion(Array(allAssets))
  }

  func addUserAsset(token: BraveWallet.BlockchainToken, completion: @escaping (Bool) -> Void) {
    assets[token.chainId]?.append(token)
  }

  func removeUserAsset(token: BraveWallet.BlockchainToken, completion: @escaping (Bool) -> Void) {
    assets[token.chainId]?.removeAll(where: { $0.contractAddress == token.contractAddress })
  }

  func setUserAssetVisible(
    token: BraveWallet.BlockchainToken,
    visible: Bool,
    completion: @escaping (Bool) -> Void
  ) {
    let chainAssets = assets[token.chainId]
    if let index = chainAssets?.firstIndex(where: { $0.contractAddress == token.contractAddress }) {
      chainAssets?[index].visible = visible
    }
  }

  func importFromExternalWallet(
    type: BraveWallet.ExternalWalletType,
    password: String,
    newPassword: String,
    completion: @escaping (Bool, String?) -> Void
  ) {
    completion(false, nil)
  }

  func defaultWallet(_ completion: @escaping (BraveWallet.DefaultWallet) -> Void) {
    completion(.braveWallet)
  }

  func hasEthereumPermission(
    _ origin: URLOrigin,
    account: String,
    completion: @escaping (Bool, Bool) -> Void
  ) {
    completion(false, false)
  }

  func resetEthereumPermission(
    _ origin: URLOrigin,
    account: String,
    completion: @escaping (Bool) -> Void
  ) {
    completion(false)
  }

  func activeOrigin(completion: @escaping (BraveWallet.OriginInfo) -> Void) {
    completion(.init())
  }

  func pendingSignMessageRequests(
    completion: @escaping ([BraveWallet.SignMessageRequest]) -> Void
  ) {
    completion([])
  }

  func pendingAddSuggestTokenRequests(
    completion: @escaping ([BraveWallet.AddSuggestTokenRequest]) -> Void
  ) {
    completion([])
  }

  func defaultBaseCurrency(completion: @escaping (String) -> Void) {
    completion(defaultCurrency.code)
  }

  func setDefaultBaseCurrency(_ currency: String) {
    defaultCurrency = CurrencyCode(code: currency)
  }

  func defaultBaseCryptocurrency(completion: @escaping (String) -> Void) {
    completion(defaultCryptocurrency)
  }

  func setDefaultBaseCryptocurrency(_ cryptocurrency: String) {
    defaultCryptocurrency = cryptocurrency
  }

  func isExternalWalletInstalled(
    type: BraveWallet.ExternalWalletType,
    completion: @escaping (Bool) -> Void
  ) {
    completion(false)
  }

  func isExternalWalletInitialized(
    type: BraveWallet.ExternalWalletType,
    completion: @escaping (Bool) -> Void
  ) {
    completion(false)
  }

  func addEthereumPermission(
    _ origin: URLOrigin,
    account: String,
    completion: @escaping (Bool) -> Void
  ) {
    completion(false)
  }

  func addObserver(_ observer: BraveWalletBraveWalletServiceObserver) {
  }

  func addTokenObserver(_ observer: BraveWalletBraveWalletServiceTokenObserver) {
  }

  func setDefaultWallet(_ defaultEthWallet: BraveWallet.DefaultWallet) {
  }

  func notifySignMessageRequestProcessed(
    approved: Bool,
    id: Int32,
    hwSignature: BraveWallet.EthereumSignatureBytes?,
    error: String?
  ) {
  }

  func notifySignMessageHardwareRequestProcessed(
    _ approved: Bool,
    id: Int32,
    signature: String,
    error: String
  ) {
  }

  func notifyAddSuggestTokenRequestsProcessed(approved: Bool, contractAddresses: [String]) {
  }

  func reset() {
  }

  func activeOrigin(_ completion: @escaping (String, String) -> Void) {
    completion("", "")
  }

  func notifyGetPublicKeyRequestProcessed(requestId: String, approved: Bool) {
  }

  func pendingGetEncryptionPublicKeyRequests() async -> [BraveWallet.GetEncryptionPublicKeyRequest]
  {
    return []
  }

  func notifyDecryptRequestProcessed(requestId: String, approved: Bool) {
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

  func hasPermission(
    accounts: [BraveWallet.AccountId],
    completion: @escaping (Bool, [BraveWallet.AccountId]) -> Void
  ) {
    completion(false, [])
  }

  func resetPermission(accountId: BraveWallet.AccountId, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func isBase58EncodedSolanaPubkey(_ key: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func eTldPlusOne(
    fromOrigin origin: URLOrigin,
    completion: @escaping (BraveWallet.OriginInfo) -> Void
  ) {
    completion(.init())
  }

  func webSitesWithPermission(coin: BraveWallet.CoinType, completion: @escaping ([String]) -> Void)
  {
    completion([])
  }

  func resetWebSitePermission(
    coin: BraveWallet.CoinType,
    formedWebsite: String,
    completion: @escaping (Bool) -> Void
  ) {
    completion(false)
  }

  func pendingSignSolTransactionsRequests(
    completion: @escaping ([BraveWallet.SignSolTransactionsRequest]) -> Void
  ) {
    completion([])
  }

  func notifySignSolTransactionsRequestProcessed(
    approved: Bool,
    id: Int32,
    hwSignatures: [BraveWallet.SolanaSignature],
    error: String?
  ) {
  }

  func base58Encode(addresses: [[NSNumber]], completion: @escaping ([String]) -> Void) {
    completion([])
  }

  func isPermissionDenied(coin: BraveWallet.CoinType, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func onOnboardingShown() {
  }

  func defaultEthereumWallet(completion: @escaping (BraveWallet.DefaultWallet) -> Void) {
    completion(.braveWallet)
  }

  func defaultSolanaWallet(completion: @escaping (BraveWallet.DefaultWallet) -> Void) {
    completion(.braveWallet)
  }

  func setDefaultEthereumWallet(defaultWallet: BraveWallet.DefaultWallet) {
  }

  func setDefaultSolanaWallet(defaultWallet: BraveWallet.DefaultWallet) {
  }

  func nftDiscoveryEnabled(completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func setNftDiscoveryEnabled(_ enabled: Bool) {
  }

  func privateWindowsEnabled(completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func isPrivateWindow(completion: @escaping (Bool) -> Void) {
  }

  func setPrivateWindowsEnabled(_ enabled: Bool) {
  }

  func chainId(forActiveOrigin coin: BraveWallet.CoinType, completion: @escaping (String) -> Void) {
    completion(BraveWallet.MainnetChainId)
  }

  func setChainIdForActiveOrigin(
    _ coin: BraveWallet.CoinType,
    chainId: String,
    completion: @escaping (Bool) -> Void
  ) {
    completion(false)
  }

  func balanceScannerSupportedChains(completion: @escaping ([String]) -> Void) {
    completion([])
  }

  func discoverEthAllowances(completion: @escaping ([BraveWallet.AllowanceInfo]) -> Void) {
    completion([])
  }

  func setAssetSpamStatus(
    token: BraveWallet.BlockchainToken,
    status: Bool,
    completion: @escaping (Bool) -> Void
  ) {
    completion(false)
  }

  func simpleHashSpamNfTs(
    walletAddress: String,
    chainIds: [BraveWallet.ChainId],
    cursor: String?,
    completion: @escaping ([BraveWallet.BlockchainToken], String?) -> Void
  ) {
    completion([], nil)
  }

  func ensureSelectedAccountForChain(
    coin: BraveWallet.CoinType,
    chainId: String,
    completion: @escaping (BraveWallet.AccountId?) -> Void
  ) {
    completion(nil)
  }

  func networkForSelectedAccountOnActiveOrigin(
    completion: @escaping (BraveWallet.NetworkInfo?) -> Void
  ) {
    completion(nil)
  }

  func setNetworkForSelectedAccountOnActiveOrigin(
    chainId: String,
    completion: @escaping (Bool) -> Void
  ) {
    completion(false)
  }

  func convertFevmToFvmAddress(
    isMainnet: Bool,
    fevmAddresses: [String],
    completion: @escaping ([String: String]) -> Void
  ) {
    completion([:])
  }

  func pendingSignMessageErrors(completion: @escaping ([BraveWallet.SignMessageError]) -> Void) {
    completion([])
  }

  func notifySignMessageErrorProcessed(errorId: String) {

  }

  func generateReceiveAddress(
    accountId: BraveWallet.AccountId,
    completion: @escaping (String?, String?) -> Void
  ) {
    completion(nil, "Error Message")
  }

  func discoverAssetsOnAllSupportedChains(bypassRateLimit: Bool) {
  }

  func transactionSimulationOptInStatus(
    completion: @escaping (BraveWallet.BlowfishOptInStatus) -> Void
  ) {
    completion(.unset)
  }

  func setTransactionSimulationOptInStatus(_ status: BraveWallet.BlowfishOptInStatus) {
  }

  func countryCode(
    completion: @escaping (String) -> Void
  ) {
  }
}
#endif
