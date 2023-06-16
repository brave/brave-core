// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Combine
import Preferences

/// The main wallet store
public class WalletStore {

  public let keyringStore: KeyringStore
  public var cryptoStore: CryptoStore?
  /// The origin of the active tab (if applicable). Used for fetching/selecting network for the DApp origin.
  public var origin: URLOrigin? {
    didSet {
      cryptoStore?.origin = origin
      keyringStore.origin = origin
    }
  }
  
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
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    ipfsApi: IpfsAPI
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
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi
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
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    ipfsApi: IpfsAPI
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
            solTxManagerProxy: solTxManagerProxy,
            ipfsApi: ipfsApi,
            origin: self.origin
          )
          if let cryptoStore = self.cryptoStore {
            Task {
              // if called in `CryptoStore.init` we may crash
              await cryptoStore.networkStore.setup()
            }
            self.onPendingRequestCancellable = cryptoStore.$pendingRequest
              .removeDuplicates()
              .sink { [weak self] _ in
                self?.onPendingRequestUpdated.send()
              }
          }
        }
      }
  }
}
