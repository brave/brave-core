// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

/// A store contains data for swap tokens
public class SwapTokenStore: ObservableObject {
  /// All  tokens
  @Published var allTokens: [BraveWallet.ERCToken] = []
  /// The current selected token to swap from. Default with nil value.
  @Published var selectedFromToken: BraveWallet.ERCToken? {
    didSet {
      if let token = selectedFromToken {
        fetchTokenBalance(for: token) { [weak self] balance in
          self?.selectedFromTokenBalance = balance
        }
        fetchTokenMarketPrice(for: token)
      }
    }
  }
  /// The current selected token to swap to. Default with nil value
  @Published var selectedToToken: BraveWallet.ERCToken? {
    didSet {
      if let token = selectedToToken {
        fetchTokenBalance(for: token) { [weak self] balance in
          self?.selectedToTokenBalance = balance
        }
      }
    }
  }
  /// The current selected token balance to swap from. Default with nil value.
  @Published var selectedFromTokenBalance: Double?
  /// The current selected token balance to swap to. Default with nil value.
  @Published var selectedToTokenBalance: Double?
  /// The current market price for selected token to swap from. Default with nil value
  @Published var selectedFromTokenPrice: Double?
  
  private let tokenRegistry: BraveWalletERCTokenRegistry
  private let rpcController: BraveWalletEthJsonRpcController
  private let assetRatioController: BraveWalletAssetRatioController
  private let swapController: BraveWalletSwapController
  private var accountInfo: BraveWallet.AccountInfo?
  
  public init(
    tokenRegistry: BraveWalletERCTokenRegistry,
    rpcController: BraveWalletEthJsonRpcController,
    assetRatioController: BraveWalletAssetRatioController,
    swapController: BraveWalletSwapController
  ) {
    self.tokenRegistry = tokenRegistry
    self.rpcController = rpcController
    self.assetRatioController = assetRatioController
    self.swapController = swapController
  }
  
  private func fetchTokenBalance(
    for token: BraveWallet.ERCToken,
    completion: @escaping (_ balance: Double?) -> Void
  ) {
    guard let account = accountInfo else {
      completion(nil)
      return
    }
    
    rpcController.balance(for: token, in: account) { balance in
      completion(balance)
    }
  }
  
  private func fetchTokenMarketPrice(for token: BraveWallet.ERCToken) {
  }
  
  func prepare(with accountInfo: BraveWallet.AccountInfo) {
    self.accountInfo = accountInfo
    
    tokenRegistry.allTokens { [self] tokens in
      let fullList = tokens + [.eth]
      allTokens = fullList.sorted(by: { $0.symbol < $1.symbol })
      
      if let fromToken = selectedFromToken { // refresh balance
        rpcController.balance(for: fromToken, in: accountInfo) { balance in
          selectedFromTokenBalance = balance
        }
      } else {
        selectedFromToken = allTokens.first(where: { $0.symbol == "ETH" })
      }
      
      rpcController.chainId { [self] chainId in
        if let toToken = selectedToToken {
          rpcController.balance(for: toToken, in: accountInfo) { balance in
            selectedToTokenBalance = balance
          }
        } else {
          if chainId == BraveWallet.MainnetChainId {
            selectedToToken = allTokens.first(where: { $0.symbol == "BAT" })
          } else if chainId == BraveWallet.RopstenChainId {
            selectedToToken = allTokens.first(where: { $0.symbol == "DAI" })
          }
        }
      }
    }
  }
}
