// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

/// The main wallet store
public class WalletStore {
  
  public let keyringStore: KeyringStore
  public let networkStore: NetworkStore
  public let portfolioStore: PortfolioStore
  
  // MARK: -
  
  private let keyringController: BraveWalletKeyringController
  private let rpcController: BraveWalletEthJsonRpcController
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioController: BraveWalletAssetRatioController
  private let swapController: BraveWalletSwapController
  let tokenRegistry: BraveWalletERCTokenRegistry
  private let transactionController: BraveWalletEthTxController
  
  public init(
    keyringController: BraveWalletKeyringController,
    rpcController: BraveWalletEthJsonRpcController,
    walletService: BraveWalletBraveWalletService,
    assetRatioController: BraveWalletAssetRatioController,
    swapController: BraveWalletSwapController,
    tokenRegistry: BraveWalletERCTokenRegistry,
    transactionController: BraveWalletEthTxController
  ) {
    self.keyringController = keyringController
    self.rpcController = rpcController
    self.walletService = walletService
    self.assetRatioController = assetRatioController
    self.swapController = swapController
    self.tokenRegistry = tokenRegistry
    self.transactionController = transactionController
    
    self.keyringStore = .init(keyringController: keyringController)
    self.networkStore = .init(rpcController: rpcController)
    self.portfolioStore = .init(
      keyringController: keyringController,
      rpcController: rpcController,
      walletService: walletService,
      assetRatioController: assetRatioController,
      tokenRegistry: tokenRegistry
    )
  }
}
