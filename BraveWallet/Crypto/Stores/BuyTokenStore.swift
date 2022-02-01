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
  private let buyAssetUrls: [String: String] = [BraveWallet.RopstenChainId: "https://faucet.ropsten.be/",
                                                BraveWallet.RinkebyChainId: "https://www.rinkeby.io/#stats",
                                                BraveWallet.GoerliChainId: "https://goerli-faucet.slock.it/",
                                                BraveWallet.KovanChainId: "https://github.com/kovan-testnet/faucet",
                                                BraveWallet.LocalhostChainId: ""]
  
  public init(
    blockchainRegistry: BraveWalletBlockchainRegistry,
    rpcService: BraveWalletJsonRpcService,
    prefilledToken: BraveWallet.BlockchainToken?
  ) {
    self.blockchainRegistry = blockchainRegistry
    self.rpcService = rpcService
    self.selectedBuyToken = prefilledToken
  }
  
  func fetchBuyUrl(account: BraveWallet.AccountInfo, amount: String, completion: @escaping (_ url: String?) -> Void) {
    guard let token = selectedBuyToken else {
      completion(nil)
      return
    }
    
    blockchainRegistry.buyUrl(BraveWallet.MainnetChainId, address: account.address, symbol: token.symbol, amount: amount) { url in
      completion(url)
    }
  }
  
  func fetchTestFaucetUrl(completion: @escaping (_ url: String?) -> Void) {
    rpcService.chainId { [self] chainId in
      completion(self.buyAssetUrls[chainId])
    }
  }
  
  func fetchBuyTokens() {
    blockchainRegistry.buyTokens(BraveWallet.MainnetChainId) { [self] tokens in
      buyTokens = tokens.sorted(by: { $0.symbol < $1.symbol })
      if selectedBuyToken == nil, let index = tokens.firstIndex(where: { $0.symbol == "BAT" }) {
        selectedBuyToken = tokens[index]
      }
    }
  }
}
