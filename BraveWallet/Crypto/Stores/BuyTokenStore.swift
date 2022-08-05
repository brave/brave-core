// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

/// A store contains data for buying tokens
public class BuyTokenStore: ObservableObject {
  /// The current selected token to buy. Default with nil value.
  @Published var selectedBuyToken: BraveWallet.BlockchainToken?
  /// All available buyable tokens
  @Published var buyTokens: [BraveWallet.BlockchainToken] = []

  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let rpcService: BraveWalletJsonRpcService

  public init(
    blockchainRegistry: BraveWalletBlockchainRegistry,
    rpcService: BraveWalletJsonRpcService,
    prefilledToken: BraveWallet.BlockchainToken?
  ) {
    self.blockchainRegistry = blockchainRegistry
    self.rpcService = rpcService
    self.selectedBuyToken = prefilledToken
    
    self.rpcService.add(self)
  }

  func fetchBuyUrl(
    chainId: String,
    account: BraveWallet.AccountInfo,
    amount: String,
    completion: @escaping (_ url: String?) -> Void
  ) {
    guard let token = selectedBuyToken else {
      completion(nil)
      return
    }

    blockchainRegistry.buyUrl(.wyre, chainId: chainId, address: account.address, symbol: token.symbol, amount: amount) { url, error  in
      completion(error != nil ? nil : url)
    }
  }

  func fetchBuyTokens(network: BraveWallet.NetworkInfo) {
    blockchainRegistry.buyTokens(.wyre, chainId: network.chainId) { [self] tokens in
      buyTokens = tokens.sorted(by: { $0.symbol < $1.symbol })
      if selectedBuyToken == nil || selectedBuyToken?.chainId != network.chainId {
        if let index = buyTokens.firstIndex(where: { network.isNativeAsset($0) }) {
          selectedBuyToken = buyTokens[safe: index]
        } else {
          selectedBuyToken = buyTokens.first
        }
      }
    }
  }
}

extension BuyTokenStore: BraveWalletJsonRpcServiceObserver {
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
    Task { @MainActor in
      let network = await rpcService.network(coin)
      fetchBuyTokens(network: network)
    }
  }
  
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
