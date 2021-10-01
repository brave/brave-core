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
  private var assets: [String: [BraveWallet.ERCToken]] = [:]
  
  func userAssets(_ chainId: String, completion: @escaping ([BraveWallet.ERCToken]) -> Void) {
    completion(assets[chainId] ?? [])
  }
  
  func addUserAsset(_ token: BraveWallet.ERCToken, chainId: String, completion: @escaping (Bool) -> Void) {
    assets[chainId]?.append(token)
  }
  
  func removeUserAsset(_ contractAddress: String, chainId: String, completion: @escaping (Bool) -> Void) {
    assets[chainId]?.removeAll(where: { $0.contractAddress == contractAddress })
  }
  
  func setUserAssetVisible(_ contractAddress: String, chainId: String, visible: Bool, completion: @escaping (Bool) -> Void) {
    let chainAssets = assets[chainId]
    if let index = chainAssets?.firstIndex(where: { $0.contractAddress == contractAddress }) {
      chainAssets?[index].visible = visible
    }
  }
  
  func isCryptoWalletsInstalled(_ completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func isMetaMaskInstalled(_ completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func `import`(fromCryptoWallets password: String, newPassword: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func `import`(fromMetaMask password: String, newPassword: String, completion: @escaping (Bool) -> Void) {
    completion(false)
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
}
