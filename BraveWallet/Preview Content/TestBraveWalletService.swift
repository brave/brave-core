// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

/// A test wallet service that implements some basic functionality for the use of SwiftUI Previews.
///
/// - note: Do not use this directly, use ``NetworkStore.previewStore``
class TestBraveWalletService: BraveWalletBraveWalletService {
  private var assets: [String: [BraveWallet.ERCToken]] = [
    BraveWallet.MainnetChainId: [.eth],
    BraveWallet.RopstenChainId: [.eth],
  ]
  private var defaultCurrency = "usd"
  private var defaultCryptocurrency = "eth"
  
  func userAssets(_ chainId: String, completion: @escaping ([BraveWallet.ERCToken]) -> Void) {
    completion(assets[chainId] ?? [])
  }
  
  func addUserAsset(_ token: BraveWallet.ERCToken, chainId: String, completion: @escaping (Bool) -> Void) {
    assets[chainId]?.append(token)
  }
  
  func removeUserAsset(_ token: BraveWallet.ERCToken, chainId: String, completion: @escaping (Bool) -> Void) {
    assets[chainId]?.removeAll(where: { $0.contractAddress == token.contractAddress })
  }
  
  func setUserAssetVisible(_ token: BraveWallet.ERCToken, chainId: String, visible: Bool, completion: @escaping (Bool) -> Void) {
    let chainAssets = assets[chainId]
    if let index = chainAssets?.firstIndex(where: { $0.contractAddress == token.contractAddress }) {
      chainAssets?[index].visible = visible
    }
  }
  
  func isCryptoWalletsInstalled(_ completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func isMetaMaskInstalled(_ completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func `import`(fromCryptoWallets password: String, newPassword: String, completion: @escaping (Bool, String?) -> Void) {
    completion(false, nil)
  }
  
  func `import`(fromMetaMask password: String, newPassword: String, completion: @escaping (Bool, String?) -> Void) {
    completion(false, nil)
  }
  
  func defaultWallet(_ completion: @escaping (BraveWallet.DefaultWallet) -> Void) {
    completion(.braveWallet)
  }
  
  func setDefaultWallet(_ defaultWallet: BraveWallet.DefaultWallet) {
    
  }
  
  func hasEthereumPermission(_ origin: String, account: String, completion: @escaping (Bool, Bool) -> Void) {
    completion(false, false)
  }
  
  func resetEthereumPermission(_ origin: String, account: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func add(_ observer: BraveWalletBraveWalletServiceObserver) {
  }
  
  func activeOrigin(_ completion: @escaping (String) -> Void) {
    completion("")
  }
  
  func pendingSignMessageRequest(_ completion: @escaping (Int32, String, String) -> Void) {
    completion(0, "", "")
  }
  
  func pendingSignMessageRequests(_ completion: @escaping ([BraveWallet.SignMessageRequest]) -> Void) {
    completion([])
  }
  
  func notifySignMessageRequestProcessed(_ approved: Bool, id: Int32) {
  }
  
  func notifySignMessageHardwareRequestProcessed(_ approved: Bool, id: Int32, signature: String, error: String) {
    // Hardware wallets not supported on iOS
  }
  
  func defaultBaseCurrency(_ completion: @escaping (String) -> Void) {
    completion(defaultCurrency)
  }
  
  func setDefaultBaseCurrency(_ currency: String) {
    defaultCurrency = currency.lowercased()
  }
  
  func defaultBaseCryptocurrency(_ completion: @escaping (String) -> Void) {
    completion(defaultCryptocurrency)
  }
  
  func setDefaultBaseCryptocurrency(_ cryptocurrency: String) {
    defaultCryptocurrency = cryptocurrency
  }
}
