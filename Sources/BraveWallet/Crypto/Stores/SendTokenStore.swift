// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Strings
import BigNumber

/// A store contains data for sending tokens
public class SendTokenStore: ObservableObject {
  /// User's asset with selected account and chain
  @Published var userAssets: [BraveWallet.BlockchainToken] = []
  /// The current selected token to send. Default with nil value.
  @Published var selectedSendToken: BraveWallet.BlockchainToken? {
    didSet {
      update() // need to update `selectedSendTokenBalance`
    }
  }
  /// The current selected token balance. Default with nil value.
  @Published var selectedSendTokenBalance: BDouble?
  /// A boolean indicates if this store is making an unapproved tx
  @Published var isMakingTx = false
  /// The destination account address
  @Published var sendAddress = "" {
    didSet {
      sendAddressUpdatedTimer?.invalidate()
      sendAddressUpdatedTimer = Timer.scheduledTimer(
        withTimeInterval: 0.25, // try not to validate for every character entered
        repeats: false,
        block: { [weak self] _ in
          self?.validateSendAddress()
        })
    }
  }
  /// An error for input send address. Nil for no error.
  @Published var addressError: AddressError?
  /// The amount the user inputs to send
  @Published var sendAmount = "" {
    didSet {
      sendAmountUpdatedTimer?.invalidate()
      sendAmountUpdatedTimer = Timer.scheduledTimer(
        withTimeInterval: 0.25, // try not to validate for every character entered
        repeats: false,
        block: { [weak self] _ in
          self?.validateBalance()
        })
    }
  }
  /// An error for input, ex insufficient balance
  @Published var sendError: SendError?
  /// If we are loading `userAssets`, `allTokens`, and `selectedSendTokenBalance`
  @Published var isLoading: Bool = false

  enum AddressError: LocalizedError {
    case sameAsFromAddress
    case contractAddress
    case notEthAddress
    case missingChecksum
    case invalidChecksum
    case notSolAddress

    var errorDescription: String? {
      switch self {
      case .sameAsFromAddress:
        return Strings.Wallet.sendWarningAddressIsOwn
      case .contractAddress:
        return Strings.Wallet.sendWarningAddressIsContract
      case .notEthAddress:
        return Strings.Wallet.sendWarningAddressNotValid
      case .missingChecksum:
        return Strings.Wallet.sendWarningAddressMissingChecksumInfo
      case .invalidChecksum:
        return Strings.Wallet.sendWarningAddressInvalidChecksum
      case .notSolAddress:
        return Strings.Wallet.sendWarningSolAddressNotValid
      }
    }
  }
  
  enum SendError: LocalizedError {
    case insufficientBalance
    
    var errorDescription: String? {
      switch self {
      case .insufficientBalance: return Strings.Wallet.insufficientBalance
      }
    }
  }

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let ethTxManagerProxy: BraveWalletEthTxManagerProxy
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private var allTokens: [BraveWallet.BlockchainToken] = []
  private var sendAddressUpdatedTimer: Timer?
  private var sendAmountUpdatedTimer: Timer?

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    prefilledToken: BraveWallet.BlockchainToken?
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.ethTxManagerProxy = ethTxManagerProxy
    self.solTxManagerProxy = solTxManagerProxy
    self.selectedSendToken = prefilledToken

    self.keyringService.add(self)
    self.rpcService.add(self)
  }
  
  func suggestedAmountTapped(_ amount: ShortcutAmountGrid.Amount) {
    var decimalPoint = 6
    var rounded = true
    if amount == .all {
      decimalPoint = Int(selectedSendToken?.decimals ?? 18)
      rounded = false
    }
    sendAmount = ((selectedSendTokenBalance ?? 0) * amount.rawValue).decimalExpansion(precisionAfterDecimalPoint: decimalPoint, rounded: rounded)
  }

  /// Cancellable for the last running `update()` Task.
  private var updateTask: Task<(), Never>?
  /// Updates the `userAssets`, `allTokens`, and `selectedSendTokenBalance`
  func update() {
    self.updateTask?.cancel()
    self.updateTask = Task { @MainActor in
      self.isLoading = true
      defer { self.isLoading = false }
      let coin = await self.walletService.selectedCoin()
      let network = await self.rpcService.network(coin)
      // fetch user assets
      let userAssets = await self.walletService.userAssets(network.chainId, coin: network.coin)
      let allTokens = await self.blockchainRegistry.allTokens(network.chainId, coin: network.coin)
      guard !Task.isCancelled else { return }
      if selectedSendToken == nil {
        self.selectedSendToken = userAssets.first
      }
      self.userAssets = userAssets
      self.allTokens = allTokens
      self.validateSendAddress() // `sendAddress` may match a token contract address
      // fetch balance for `selectedSendToken`
      guard let selectedAccount = await self.keyringService.selectedAccount(coin),
            let selectedSendToken = self.selectedSendToken else {
        self.selectedSendTokenBalance = nil // no selected account, or send token is nil
        return
      }
      let balance = await self.rpcService.balance(
        for: selectedSendToken,
        in: selectedAccount,
        network: network,
        decimalFormatStyle: .decimals(precision: Int(selectedSendToken.decimals))
      )
      guard !Task.isCancelled else { return }
      self.selectedSendTokenBalance = balance
      self.validateBalance()
    }
  }

  private func makeEIP1559Tx(
    chainId: String,
    baseData: BraveWallet.TxData,
    from address: String,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    let eip1559Data = BraveWallet.TxData1559(baseData: baseData, chainId: chainId, maxPriorityFeePerGas: "", maxFeePerGas: "", gasEstimation: nil)
    let txDataUnion = BraveWallet.TxDataUnion(ethTxData1559: eip1559Data)
    self.txService.addUnapprovedTransaction(txDataUnion, from: address, origin: nil, groupId: nil) { success, txMetaId, errorMessage in
      completion(success, errorMessage)
    }
  }

  /// Validate the `sendAddress`
  private func validateSendAddress() {
    Task { @MainActor in
      let coin = await self.walletService.selectedCoin()
      guard let sendFromAddress = await self.keyringService.selectedAccount(coin),
            !sendAddress.isEmpty,
            let token = selectedSendToken else {
        return
      }
      let normalizedSendToAddress = sendAddress.lowercased()
      if token.coin == .eth {
        if !sendAddress.isETHAddress {
          // 1. check if send address is a valid eth address
          addressError = .notEthAddress
        } else if sendFromAddress.lowercased() == normalizedSendToAddress {
          // 2. check if send address is the same as the from address
          addressError = .sameAsFromAddress
        } else if (userAssets.first(where: { $0.contractAddress.lowercased() == normalizedSendToAddress }) != nil)
                    || (allTokens.first(where: { $0.contractAddress.lowercased() == normalizedSendToAddress }) != nil) {
          // 3. check if send address is a contract address
          addressError = .contractAddress
        } else {
          let checksumAddress = await keyringService.checksumEthAddress(sendAddress)
          if sendAddress == checksumAddress {
            // 4. check if send address is the same as the checksum address from the `KeyringService`
            addressError = nil
          } else if sendAddress.removingHexPrefix.lowercased() == sendAddress.removingHexPrefix || sendAddress.removingHexPrefix.uppercased() == sendAddress.removingHexPrefix {
            // 5. check if send address has each of the alphabetic character as uppercase, or has each of
            // the alphabeic character as lowercase
            addressError = .missingChecksum
          } else {
            // 6. send address has mixed with uppercase and lowercase and does not match with the checksum address
            addressError = .invalidChecksum
          }
        }
      } else if token.coin == .sol {
        let isValid = await walletService.isBase58EncodedSolanaPubkey(sendAddress)
        if !isValid {
          addressError = .notSolAddress
        } else if sendFromAddress.lowercased() == normalizedSendToAddress {
          addressError = .sameAsFromAddress
        } else {
          addressError = nil
        }
      }
    }
  }
  
  /// Validate `selectedSendTokenBalance` against the `sendAmount`
  private func validateBalance() {
    guard let selectedSendToken = self.selectedSendToken,
          let balance = selectedSendTokenBalance,
          case let sendAmount = (selectedSendToken.isErc721 || selectedSendToken.isNft) ? "1" : self.sendAmount,
          let sendAmount = BDouble(sendAmount) else {
      sendError = nil
      return
    }
    sendError = balance < sendAmount ? .insufficientBalance : nil
  }

  func sendToken(
    amount: String,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    guard let token = self.selectedSendToken else {
      completion(false, "An Internal Error")
      return
    }
    let amount = (token.isErc721 || token.isNft) ? "1" : amount
    walletService.selectedCoin { [weak self] coin in
      guard let self = self else { return }
      self.keyringService.selectedAccount(coin) { selectedAccount in
        guard let selectedAccount = selectedAccount else {
          completion(false, "An Internal Error")
          return
        }
        switch coin {
        case .eth:
          self.sendTokenOnEth(amount: amount, token: token, fromAddress: selectedAccount, completion: completion)
        case .sol:
          self.sendTokenOnSol(amount: amount, token: token, fromAddress: selectedAccount, completion: completion)
        default:
          break
        }
      }
    }
  }

  func sendTokenOnEth(
    amount: String,
    token: BraveWallet.BlockchainToken,
    fromAddress: String,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    guard let weiHexString = weiFormatter.weiString(from: amount.normalizedDecimals, radix: .hex, decimals: Int(token.decimals)) else {
      completion(false, "An Internal Error")
      return
    }

    isMakingTx = true
    rpcService.network(.eth) { [weak self] network in
      guard let self = self else { return }
      if network.isNativeAsset(token) {
        let baseData = BraveWallet.TxData(nonce: "", gasPrice: "", gasLimit: "", to: self.sendAddress, value: "0x\(weiHexString)", data: .init())
        if network.isEip1559 {
          self.makeEIP1559Tx(chainId: network.chainId, baseData: baseData, from: fromAddress) { success, errorMessage  in
            self.isMakingTx = false
            completion(success, errorMessage)
          }
        } else {
          let txDataUnion = BraveWallet.TxDataUnion(ethTxData: baseData)
          self.txService.addUnapprovedTransaction(txDataUnion, from: fromAddress, origin: nil, groupId: nil) { success, txMetaId, errorMessage in
            self.isMakingTx = false
            completion(success, errorMessage)
          }
        }
      } else if token.isErc721 {
        self.ethTxManagerProxy.makeErc721Transfer(fromData: fromAddress, to: self.sendAddress, tokenId: token.tokenId, contractAddress: token.contractAddress) { success, data in
          guard success else {
            completion(false, nil)
            return
          }
          let baseData = BraveWallet.TxData(nonce: "", gasPrice: "", gasLimit: "", to: token.contractAddress, value: "0x0", data: data)
          let txDataUnion = BraveWallet.TxDataUnion(ethTxData: baseData)
          self.txService.addUnapprovedTransaction(txDataUnion, from: fromAddress, origin: nil, groupId: nil) { success, txMetaId, errorMessage in
            self.isMakingTx = false
            completion(success, errorMessage)
          }
        }
      } else {
        self.ethTxManagerProxy.makeErc20TransferData(self.sendAddress, amount: "0x\(weiHexString)") { success, data in
          guard success else {
            completion(false, nil)
            return
          }
          let baseData = BraveWallet.TxData(nonce: "", gasPrice: "", gasLimit: "", to: token.contractAddress, value: "0x0", data: data)
          if network.isEip1559 {
            self.makeEIP1559Tx(chainId: network.chainId, baseData: baseData, from: fromAddress) { success, errorMessage  in
              self.isMakingTx = false
              completion(success, errorMessage)
            }
          } else {
            let txDataUnion = BraveWallet.TxDataUnion(ethTxData: baseData)
            self.txService.addUnapprovedTransaction(txDataUnion, from: fromAddress, origin: nil, groupId: nil) { success, txMetaId, errorMessage in
              self.isMakingTx = false
              completion(success, errorMessage)
            }
          }
        }
      }
    }
  }
  
  private func sendTokenOnSol(
    amount: String,
    token: BraveWallet.BlockchainToken,
    fromAddress: String,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    guard let amount = WeiFormatter.decimalToAmount(amount.normalizedDecimals, tokenDecimals: Int(token.decimals)) else {
      completion(false, "An Internal Error")
      return
    }

    rpcService.network(.sol) { [weak self] network in
      guard let self = self else { return }
      if network.isNativeAsset(token) {
        self.solTxManagerProxy.makeSystemProgramTransferTxData(
          fromAddress,
          to: self.sendAddress,
          lamports: amount
        ) { solTxData, error, errMsg in
          guard let solanaTxData = solTxData else {
            completion(false, errMsg)
            return
          }
          let txDataUnion = BraveWallet.TxDataUnion(solanaTxData: solanaTxData)
          self.txService.addUnapprovedTransaction(txDataUnion, from: fromAddress, origin: nil, groupId: nil) { success, txMetaId, errMsg in
            completion(success, errMsg)
          }
        }
      } else {
        self.solTxManagerProxy.makeTokenProgramTransferTxData(
          token.contractAddress,
          fromWalletAddress: fromAddress,
          toWalletAddress: self.sendAddress,
          amount: amount
        ) { solTxData, error, errMsg in
          guard let solanaTxData = solTxData else {
            completion(false, errMsg)
            return
          }
          let txDataUnion = BraveWallet.TxDataUnion(solanaTxData: solanaTxData)
          self.txService.addUnapprovedTransaction(txDataUnion, from: fromAddress, origin: nil, groupId: nil) { success, txMetaId, errorMessage in
            completion(success, errorMessage)
          }
        }
      }
    }
  }
}

extension SendTokenStore: BraveWalletKeyringServiceObserver {
  public func keyringReset() {
  }

  public func keyringCreated(_ keyringId: String) {
  }

  public func keyringRestored(_ keyringId: String) {
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

  public func selectedAccountChanged(_ coinType: BraveWallet.CoinType) {
    selectedSendTokenBalance = nil
    addressError = nil
    update() // `selectedSendTokenBalance` needs updated for new account
    validateSendAddress() // `sendAddress` may equal selected account address
  }
}

extension SendTokenStore: BraveWalletJsonRpcServiceObserver {
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
    selectedSendToken = nil
    selectedSendTokenBalance = nil
    update() // `selectedSendToken` & `selectedSendTokenBalance` need updated for new chain
    validateSendAddress() // `sendAddress` may no longer be valid if coin type changed
  }

  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }

  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
