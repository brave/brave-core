// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

/// A store contains data for sending tokens
public class SendTokenStore: ObservableObject {
  /// User's asset with selected account and chain
  @Published var userAssets: [BraveWallet.ERCToken] = []
  /// The current selected token to send. Default with nil value.
  @Published var selectedSendToken: BraveWallet.ERCToken? {
    didSet {
      fetchAssetBalance()
    }
  }
  /// The current selected token balance. Default with nil value.
  @Published var selectedSendTokenBalance: Double?
  
  private let keyringController: BraveWalletKeyringController
  private let rpcController: BraveWalletEthJsonRpcController
  private let walletService: BraveWalletBraveWalletService
  private let transactionController: BraveWalletEthTxController
  
  public init(
    keyringController: BraveWalletKeyringController,
    rpcController: BraveWalletEthJsonRpcController,
    walletService: BraveWalletBraveWalletService,
    transactionController: BraveWalletEthTxController
  ) {
    self.keyringController = keyringController
    self.rpcController = rpcController
    self.walletService = walletService
    self.transactionController = transactionController
    
    self.keyringController.add(self)
    self.rpcController.add(self)
  }
  
  func fetchAssets() {
    rpcController.chainId { [self] chainId in
      walletService.userAssets(chainId) { tokens in
        userAssets = tokens
        
        if let selectedToken = selectedSendToken {
          if tokens.isEmpty {
            selectedSendToken = nil
          } else if let token = tokens.first(where: { $0.id == selectedToken.id }) {
            selectedSendToken = token
          }
        } else {
          selectedSendToken = tokens.first
        }
      }
    }
  }
  
  private func fetchAssetBalance() {
    guard let token = selectedSendToken else {
      selectedSendTokenBalance = nil
      return
    }
    
    rpcController.chainId { [self] chainId in
      walletService.userAssets(chainId) { tokens in
        guard let index = tokens.firstIndex(where: { $0.id == token.id}) else {
          self.selectedSendTokenBalance = nil
          return
        }
        self.fetchBalance(for: tokens[index])
      }
    }
  }
  
  private func fetchBalance(for token: BraveWallet.ERCToken) {
    keyringController.selectedAccount { [self] accountAddress in
      guard let accountAddress = accountAddress else {
        self.selectedSendTokenBalance = nil
        return
      }
      
      let balanceFormatter = WeiFormatter(decimalFormatStyle: .balance)
      func updateBalance(_ success: Bool, _ balance: String) {
        guard success, let decimalString = balanceFormatter.decimalString(
          for: balance.removingHexPrefix,
             radix: .hex,
             decimals: Int(token.decimals)
        ), !decimalString.isEmpty, let decimal = Double(decimalString) else {
          return
        }
        selectedSendTokenBalance = decimal
      }
      
      // Get balance for ETH token
      if token.isETH {
        self.rpcController.balance(accountAddress) { success, balance in
          guard success else {
            self.selectedSendTokenBalance = nil
            return
          }
          updateBalance(success, balance)
        }
      }
      // Get balance for erc20 token
      else if token.isErc20 {
        self.rpcController.erc20TokenBalance(token.contractAddress,
                                        address: accountAddress) { success, balance in
          guard success else {
            self.selectedSendTokenBalance = nil
            return
          }
          updateBalance(success, balance)
        }
      }
      // Get balance for erc721 token
      else if token.isErc721 {
        self.rpcController.erc721TokenBalance(token.contractAddress,
                                              tokenId: token.id,
                                              accountAddress: accountAddress) { success, balance in
          guard success else {
            self.selectedSendTokenBalance = nil
            return
          }
          updateBalance(success, balance)
        }
      }
    }
  }
  
  func sendToken(
    from account: BraveWallet.AccountInfo,
    to address: String,
    amount: String,
    completion: @escaping (_ success: Bool) -> Void
  ) {
    let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    guard let token = selectedSendToken, let weiHexString = weiFormatter.weiString(from: amount, radix: .hex, decimals: 18) else { return }
    
    if token.isETH {
      let data = BraveWallet.TxData(nonce: "", gasPrice: "", gasLimit: "", to: address, value: "0x\(weiHexString)", data: .init())
      transactionController.addUnapprovedTransaction(data, from: account.address) { success, txMetaId, errorMessage in
        completion(success)
      }
    } else {
      transactionController.makeErc20TransferData(account.address, amount: "0x\(weiHexString)") { [self] success, data in
        guard success else {
          completion(false)
          return
        }
        rpcController.chainId { [self] chainId in
          let txData = BraveWallet.TxData(nonce: "", gasPrice: "", gasLimit: "", to: token.contractAddress, value: "0x0", data: data)
          transactionController.addUnapprovedTransaction(txData, from: account.address) { success, txMetaId, errorMessage in
            completion(success)
          }
        }
      }
    }
  }
}

extension SendTokenStore: BraveWalletKeyringControllerObserver {
  public func keyringCreated() {
  }
  
  public func keyringRestored() {
  }
  
  public func locked() {
  }
  
  public func unlocked() {
  }
  
  public func backedUp() {
  }
  
  public func accountsChanged() {
  }
  
  public func autoLockMinutesChanged() {
  }
  
  public func selectedAccountChanged() {
    fetchAssetBalance()
  }
}

extension SendTokenStore: BraveWalletEthJsonRpcControllerObserver {
  public func chainChangedEvent(_ chainId: String) {
    fetchAssets()
  }
  
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
