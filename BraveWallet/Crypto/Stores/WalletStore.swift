// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Combine

/// The main wallet store
public class WalletStore {
  
  public let keyringStore: KeyringStore
  public var cryptoStore: CryptoStore?
  
  // MARK: -
  
  private var cancellable: AnyCancellable?
  
  public init(
    keyringController: BraveWalletKeyringController,
    rpcController: BraveWalletEthJsonRpcController,
    walletService: BraveWalletBraveWalletService,
    assetRatioController: BraveWalletAssetRatioController,
    swapController: BraveWalletSwapController,
    tokenRegistry: BraveWalletERCTokenRegistry,
    transactionController: BraveWalletEthTxController
  ) {
    self.keyringStore = .init(keyringController: keyringController)
    self.setUp(
      keyringController: keyringController,
      rpcController: rpcController,
      walletService: walletService,
      assetRatioController: assetRatioController,
      swapController: swapController,
      tokenRegistry: tokenRegistry,
      transactionController: transactionController
    )
  }
  
  private func setUp(
    keyringController: BraveWalletKeyringController,
    rpcController: BraveWalletEthJsonRpcController,
    walletService: BraveWalletBraveWalletService,
    assetRatioController: BraveWalletAssetRatioController,
    swapController: BraveWalletSwapController,
    tokenRegistry: BraveWalletERCTokenRegistry,
    transactionController: BraveWalletEthTxController
  ) {
    self.cancellable = self.keyringStore.$keyring
      .map(\.isDefaultKeyringCreated)
      .removeDuplicates()
      .sink { [weak self] isDefaultKeyringCreated in
        guard let self = self else { return }
        if !isDefaultKeyringCreated, self.cryptoStore != nil {
          self.cryptoStore = nil
        } else if isDefaultKeyringCreated, self.cryptoStore == nil {
          self.cryptoStore = CryptoStore(
            keyringController: keyringController,
            rpcController: rpcController,
            walletService: walletService,
            assetRatioController: assetRatioController,
            swapController: swapController,
            tokenRegistry: tokenRegistry,
            transactionController: transactionController
          )
        }
    }
  }
}
