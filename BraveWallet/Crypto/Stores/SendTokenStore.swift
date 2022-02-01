// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Shared

/// A store contains data for sending tokens
public class SendTokenStore: ObservableObject {
  /// User's asset with selected account and chain
  @Published var userAssets: [BraveWallet.BlockchainToken] = []
  /// The current selected token to send. Default with nil value.
  @Published var selectedSendToken: BraveWallet.BlockchainToken? {
    didSet {
      fetchAssetBalance()
    }
  }
  /// The current selected token balance. Default with nil value.
  @Published var selectedSendTokenBalance: Double?
  /// A boolean indicates if this store is making an unapproved tx
  @Published var isMakingTx = false
  /// The destination account address
  @Published var sendAddress = "" {
    didSet {
      timer?.invalidate()
      timer = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: false, block: { [weak self] _ in
        self?.validateSendAddress()
      })
    }
  }
  /// An error for input send address. Nil for no error.
  @Published var addressError: AddressError?
  
  enum AddressError: LocalizedError {
    case sameAsFromAddress
    case contractAddress
    case notEthAddress
    
    var errorDescription: String? {
      switch self {
      case .sameAsFromAddress:
        return Strings.Wallet.sendWarningAddressIsOwn
      case .contractAddress:
        return Strings.Wallet.sendWarningAddressIsContract
      case .notEthAddress:
        return Strings.Wallet.sendWarningAddressNotValid
      }
    }
  }
  
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let txService: BraveWalletEthTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private var allTokens: [BraveWallet.BlockchainToken] = []
  private var currentAccountAddress: String?
  private var timer: Timer?
  
  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    txService: BraveWalletEthTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    prefilledToken: BraveWallet.BlockchainToken?
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.selectedSendToken = prefilledToken
    
    self.keyringService.add(self)
    self.rpcService.add(self)
    
    self.keyringService.selectedAccount { address in
      self.currentAccountAddress = address
    }
  }
  
  func fetchAssets() {
    rpcService.network { [weak self] network in
      guard let self = self else { return }
      self.walletService.userAssets(network.chainId) { tokens in
        self.userAssets = tokens
        
        if let selectedToken = self.selectedSendToken {
          if tokens.isEmpty {
            self.selectedSendToken = nil
          } else if let token = tokens.first(where: { $0.id == selectedToken.id }) {
            self.selectedSendToken = token
          }
        } else {
          self.selectedSendToken = tokens.first
        }
      }
      
      // store tokens in `allTokens` for address validation
      self.blockchainRegistry.allTokens(BraveWallet.MainnetChainId) { tokens in
        self.allTokens = tokens + [network.nativeToken]
      }
    }
  }
  
  private func fetchAssetBalance() {
    guard let token = selectedSendToken else {
      selectedSendTokenBalance = nil
      return
    }
    
    rpcService.chainId { [self] chainId in
      walletService.userAssets(chainId) { tokens in
        guard let index = tokens.firstIndex(where: { $0.id == token.id}) else {
          self.selectedSendTokenBalance = nil
          return
        }
        self.fetchBalance(for: tokens[index])
      }
    }
  }
  
  private func fetchBalance(for token: BraveWallet.BlockchainToken) {
    keyringService.selectedAccount { [self] accountAddress in
      guard let accountAddress = accountAddress else {
        self.selectedSendTokenBalance = nil
        return
      }
      
      let balanceFormatter = WeiFormatter(decimalFormatStyle: .balance)
      func updateBalance(_ status: BraveWallet.ProviderError, _ balance: String) {
        guard status == .success, let decimalString = balanceFormatter.decimalString(
          for: balance.removingHexPrefix,
             radix: .hex,
             decimals: Int(token.decimals)
        ), !decimalString.isEmpty, let decimal = Double(decimalString) else {
          return
        }
        selectedSendTokenBalance = decimal
      }
      
      self.rpcService.network { network in
        // Get balance for ETH token
        if token.symbol == network.symbol {
          self.rpcService.balance(accountAddress, coin: .eth) { balance, status, _ in
            guard status == .success else {
              self.selectedSendTokenBalance = nil
              return
            }
            updateBalance(status, balance)
          }
        }
        // Get balance for erc20 token
        else if token.isErc20 {
          self.rpcService.erc20TokenBalance(token.contractAddress,
                                          address: accountAddress) { balance, status, _ in
            guard status == .success else {
              self.selectedSendTokenBalance = nil
              return
            }
            updateBalance(status, balance)
          }
        }
        // Get balance for erc721 token
        else if token.isErc721 {
          self.rpcService.erc721TokenBalance(token.contractAddress,
                                                tokenId: token.id,
                                                accountAddress: accountAddress) { balance, status, _ in
            guard status == .success else {
              self.selectedSendTokenBalance = nil
              return
            }
            updateBalance(status, balance)
          }
        }
      }
    }
  }
  
  private func makeEIP1559Tx(
    chainId: String,
    baseData: BraveWallet.TxData,
    from address: String,
    completion: @escaping (_ success: Bool) -> Void
  ) {
    let eip1559Data = BraveWallet.TxData1559(baseData: baseData, chainId: chainId, maxPriorityFeePerGas: "", maxFeePerGas: "", gasEstimation: nil)
    self.txService.addUnapproved1559Transaction(eip1559Data, from: address) { success, txMetaId, errorMessage in
      completion(success)
    }
  }
  
  private func validateSendAddress() {
    guard !sendAddress.isEmpty else {
      addressError = nil
      return
    }
    let normalizedSendAddress = sendAddress.lowercased()
    if !sendAddress.isETHAddress {
      addressError = .notEthAddress
    } else if currentAccountAddress?.lowercased() == normalizedSendAddress {
      addressError = .sameAsFromAddress
    } else if (userAssets.first(where: { $0.contractAddress.lowercased() == normalizedSendAddress }) != nil)
                || (allTokens.first(where: { $0.contractAddress.lowercased() == normalizedSendAddress }) != nil) {
      addressError = .contractAddress
    } else {
      addressError = nil
    }
  }
  
  func sendToken(
    amount: String,
    completion: @escaping (_ success: Bool) -> Void
  ) {
    let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    guard
      let token = selectedSendToken,
      let weiHexString = weiFormatter.weiString(from: amount, radix: .hex, decimals: Int(token.decimals)),
      let fromAddress = currentAccountAddress
    else { return }
    
    isMakingTx = true
    rpcService.network { [weak self] network in
      guard let self = self else { return }

      if token.symbol == network.symbol {
        let baseData = BraveWallet.TxData(nonce: "", gasPrice: "", gasLimit: "", to: self.sendAddress, value: "0x\(weiHexString)", data: .init())
        if network.isEip1559 {
          self.makeEIP1559Tx(chainId: network.chainId, baseData: baseData, from: fromAddress) { success in
            self.isMakingTx = false
            completion(success)
          }
        } else {
          self.txService.addUnapprovedTransaction(baseData, from: fromAddress) { success, txMetaId, errorMessage in
            self.isMakingTx = false
            completion(success)
          }
        }
      } else {
        self.txService.makeErc20TransferData(self.sendAddress, amount: "0x\(weiHexString)") { success, data in
          guard success else {
            completion(false)
            return
          }
          let baseData = BraveWallet.TxData(nonce: "", gasPrice: "", gasLimit: "", to: token.contractAddress, value: "0x0", data: data)
          if network.isEip1559 {
            self.makeEIP1559Tx(chainId: network.chainId, baseData: baseData, from: fromAddress) { success in
              self.isMakingTx = false
              completion(success)
            }
          } else {
            self.txService.addUnapprovedTransaction(baseData, from: fromAddress) { success, txMetaId, errorMessage in
              self.isMakingTx = false 
              completion(success)
            }
          }
        }
      }
    }
  }
  
  #if DEBUG
  func setUpTest() {
    currentAccountAddress = "test-current-account-address"
    sendAddress = "test-send-address"
  }
  #endif
}

extension SendTokenStore: BraveWalletKeyringServiceObserver {
  public func keyringReset() {
  }
  
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
    keyringService.selectedAccount { [weak self] address in
      self?.currentAccountAddress = address
      self?.validateSendAddress()
    }
  }
}

extension SendTokenStore: BraveWalletJsonRpcServiceObserver {
  public func chainChangedEvent(_ chainId: String) {
    fetchAssets()
  }
  
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
