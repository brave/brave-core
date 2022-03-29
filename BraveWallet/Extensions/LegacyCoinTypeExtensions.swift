// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

// These helpers are temporary and should be removed when Solana support is added

extension BraveWalletJsonRpcService {
  func setNetwork(_ chainId: String, completion: @escaping (Bool) -> Void) {
    setNetwork(chainId, coin: .eth, completion: completion)
  }
  
  func network(_ completion: @escaping (BraveWallet.NetworkInfo) -> Void) {
    network(.eth, completion: completion)
  }
  
  func allNetworks(_ completion: @escaping ([BraveWallet.NetworkInfo]) -> Void) {
    allNetworks(.eth, completion: completion)
  }
  
  func chainId(_ completion: @escaping (String) -> Void) {
    chainId(.eth, completion: completion)
  }
  
  func balance(_ address: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    balance(address, coin: .eth, chainId: chainId, completion: completion)
  }
}

extension BraveWallet.NetworkInfo {
  var isEip1559: Bool {
    if coin == .eth, let ethData = data?.ethData {
      return ethData.isEip1559
    }
    return false
  }
}
