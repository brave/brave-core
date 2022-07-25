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
  
  public let onPendingRequestUpdated = PassthroughSubject<Void, Never>()

  // MARK: -

  private var cancellable: AnyCancellable?
  private var onPendingRequestCancellable: AnyCancellable?

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    swapService: BraveWalletSwapService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    txService: BraveWalletTxService,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  ) {
    self.keyringStore = .init(keyringService: keyringService, walletService: walletService, rpcService: rpcService)
    self.setUp(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      swapService: swapService,
      blockchainRegistry: blockchainRegistry,
      txService: txService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy
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
    ethTxManagerProxy: BraveWalletEthTxManagerProxy,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  ) {
    self.cancellable = self.keyringStore.$defaultKeyring
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
            ethTxManagerProxy: ethTxManagerProxy,
            solTxManagerProxy: solTxManagerProxy
          )
          self.onPendingRequestCancellable = self.cryptoStore?.$pendingRequest
            .removeDuplicates()
            .sink { [weak self] _ in
              self?.onPendingRequestUpdated.send()
            }
        }
      }
  }
}
