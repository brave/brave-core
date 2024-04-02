// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Combine
import Foundation
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

  var isPresentingWalletPanel: Bool = false {
    didSet {
      if oldValue, !isPresentingWalletPanel {  // dismiss
        if !isPresentingFullWallet {  // both full wallet and wallet panel are dismissed
          self.tearDown()
        } else {
          // dismiss panel to present full screen. observer should be setup")
          self.setupObservers()
        }
      } else if !oldValue, isPresentingWalletPanel {  // present
        self.setupObservers()
      }
    }
  }
  var isPresentingFullWallet: Bool = false {
    didSet {
      if oldValue, !isPresentingFullWallet {  // dismiss
        if !isPresentingWalletPanel {  // both panel and full wallet are dismissed
          self.tearDown()
        } else {
          // panel is still visible, do not tear down
        }
      } else if !oldValue, isPresentingFullWallet {  // present
        if isPresentingWalletPanel {
          // observers should be setup when wallet panel is presented
        } else {
          // either open from browser settings or from wallet panel
          self.setupObservers()
        }
      }
    }
  }

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
    ipfsApi: IpfsAPI,
    walletP3A: BraveWalletBraveWalletP3A,
    bitcoinWalletService: BraveWalletBitcoinWalletService
  ) {
    self.keyringStore = .init(
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      walletP3A: walletP3A
    )
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
      ipfsApi: ipfsApi,
      walletP3A: walletP3A,
      bitcoinWalletService: bitcoinWalletService
    )
  }

  public func setupObservers() {
    keyringStore.setupObservers()
    cryptoStore?.setupObservers()
  }

  public func tearDown() {
    keyringStore.tearDown()
    cryptoStore?.tearDown()
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
    ipfsApi: IpfsAPI,
    walletP3A: BraveWalletBraveWalletP3A,
    bitcoinWalletService: BraveWalletBitcoinWalletService
  ) {
    self.cancellable = self.keyringStore.$isWalletCreated
      .removeDuplicates()
      .sink { [weak self] isWalletCreated in
        guard let self = self else { return }
        if !isWalletCreated, self.cryptoStore != nil {
          // only tear down `CryptoStore` since we still need to listen
          // default keyring creation if user didn't dismiss the wallet after reset
          self.cryptoStore?.tearDown()
          self.cryptoStore = nil
        } else if isWalletCreated, self.cryptoStore == nil {
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
            walletP3A: walletP3A,
            bitcoinWalletService: bitcoinWalletService,
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

protocol WalletObserverStore: AnyObject {
  var isObserving: Bool { get }
  func tearDown()
  func setupObservers()
}

extension WalletObserverStore {
  func tearDown() {
  }
  func setupObservers() {
  }
}
