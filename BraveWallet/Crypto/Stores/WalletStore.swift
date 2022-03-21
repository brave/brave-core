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
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    swapService: BraveWalletSwapService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    txService: BraveWalletTxService,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy
  ) {
    self.keyringStore = .init(keyringService: keyringService)
    self.setUp(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      swapService: swapService,
      blockchainRegistry: blockchainRegistry,
      txService: txService,
      ethTxManagerProxy: ethTxManagerProxy
    )
  }

  private func setUp(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    swapService: BraveWalletSwapService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    txService: BraveWalletTxService,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy
  ) {
    self.cancellable = self.keyringStore.$keyring
      .map(\.isKeyringCreated)
      .removeDuplicates()
      .sink { [weak self] isDefaultKeyringCreated in
        guard let self = self else { return }
        if !isDefaultKeyringCreated, self.cryptoStore != nil {
          self.cryptoStore = nil
        } else if isDefaultKeyringCreated, self.cryptoStore == nil {
          self.cryptoStore = CryptoStore(
            keyringService: keyringService,
            rpcService: rpcService,
            walletService: walletService,
            assetRatioService: assetRatioService,
            swapService: swapService,
            blockchainRegistry: blockchainRegistry,
            txService: txService,
            ethTxManagerProxy: ethTxManagerProxy
          )
        }
      }
  }
}
