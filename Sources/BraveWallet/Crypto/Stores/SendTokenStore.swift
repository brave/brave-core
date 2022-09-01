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
      fetchAssetBalance()
      validateSendAddress()
    }
  }
  /// The current selected token balance. Default with nil value.
  @Published var selectedSendTokenBalance: BDouble?
  /// A boolean indicates if this store is making an unapproved tx
  @Published var isMakingTx = false
  /// The destination account address
  @Published var sendAddress = "" {
    didSet {
      timer?.invalidate()
      timer = Timer.scheduledTimer(
        withTimeInterval: 0.25, repeats: false,
        block: { [weak self] _ in
          self?.validateSendAddress()
        })
    }
  }
  /// An error for input send address. Nil for no error.
  @Published var addressError: AddressError?
  /// The amount the user inputs to send
  @Published var sendAmount = ""

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

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let ethTxManagerProxy: BraveWalletEthTxManagerProxy
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private var allTokens: [BraveWallet.BlockchainToken] = []
  private var currentAccountAddress: String?
  private var timer: Timer?

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

    self.walletService.selectedCoin { coin in
      self.keyringService.selectedAccount(coin) { address in
        self.currentAccountAddress = address
      }
    }
  }

  func fetchAssets() {
    walletService.selectedCoin { [weak self] coin in
      guard let self = self else { return }
      self.rpcService.network(coin) { network in
        self.walletService.userAssets(network.chainId, coin: network.coin) { tokens in
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
        self.blockchainRegistry.allTokens(network.chainId, coin: network.coin) { tokens in
          self.allTokens = tokens + [network.nativeToken]
        }
      }
    }
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

  private func fetchAssetBalance() {
    guard let token = selectedSendToken else {
      selectedSendTokenBalance = nil
      return
    }

    walletService.selectedCoin { [weak self] coin in
      guard let self = self else { return }
      self.rpcService.network(coin) { network in
        self.walletService.userAssets(network.chainId, coin: network.coin) { tokens in
          guard let index = tokens.firstIndex(where: { $0.id == token.id }) else {
            self.selectedSendTokenBalance = nil
            return
          }
          self.fetchBalance(for: tokens[index])
        }
      }
    }
  }

  private func fetchBalance(for token: BraveWallet.BlockchainToken) {
    walletService.selectedCoin { [weak self] coin in
      guard let self = self else { return }
      self.keyringService.selectedAccount(coin) { accountAddress in
        guard let accountAddress = accountAddress else {
          self.selectedSendTokenBalance = nil
          return
        }
        
        self.rpcService.balance(
          for: token,
          in: accountAddress,
          with: coin,
          decimalFormatStyle: .decimals(precision: Int(token.decimals))
        ) { balance in
          self.selectedSendTokenBalance = balance
        }
      }
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

  private func validateSendAddress() {
    guard !sendAddress.isEmpty, let token = selectedSendToken else {
      addressError = nil
      return
    }
    let normalizedSendAddress = sendAddress.lowercased()
    if token.coin == .eth {
      if !sendAddress.isETHAddress {
        // 1. check if send address is a valid eth address
        addressError = .notEthAddress
      } else if currentAccountAddress?.lowercased() == normalizedSendAddress {
        // 2. check if send address is the same as the from address
        addressError = .sameAsFromAddress
      } else if (userAssets.first(where: { $0.contractAddress.lowercased() == normalizedSendAddress }) != nil)
                  || (allTokens.first(where: { $0.contractAddress.lowercased() == normalizedSendAddress }) != nil) {
        // 3. check if send address is a contract address
        addressError = .contractAddress
      } else {
        keyringService.checksumEthAddress(sendAddress) { [self] checksumAddress in
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
      }
    } else if token.coin == .sol {
      walletService.isBase58EncodedSolanaPubkey(sendAddress) { [self] valid in
        if !valid {
          addressError = .notSolAddress
        } else if currentAccountAddress?.lowercased() == normalizedSendAddress {
          addressError = .sameAsFromAddress
        } else {
          addressError = nil
        }
      }
    }
  }

  func sendToken(
    amount: String,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    walletService.selectedCoin { [weak self] coin in
      guard let self = self else { return }
      switch coin {
      case .eth:
        self.sendTokenOnEth(amount: amount, completion: completion)
      case .sol:
        self.sendTokenOnSol(amount: amount, completion: completion)
      default:
        break
      }
    }
  }

  func sendTokenOnEth(
    amount: String,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    guard
      let token = selectedSendToken,
      let weiHexString = weiFormatter.weiString(from: amount.normalizedDecimals, radix: .hex, decimals: Int(token.decimals)),
      let fromAddress = currentAccountAddress
    else { return }

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
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    guard let token = selectedSendToken,
          let fromAddress = currentAccountAddress,
          let amount = WeiFormatter.decimalToAmount(amount.normalizedDecimals, tokenDecimals: Int(token.decimals))
    else {
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
    fetchAssetBalance()
    keyringService.selectedAccount(coinType) { [weak self] address in
      self?.currentAccountAddress = address
      self?.validateSendAddress()
    }
  }
}

extension SendTokenStore: BraveWalletJsonRpcServiceObserver {
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
    // nil `selectedSendToken` to force refresh `selectedSendToken` in `fetchAssets()`
    selectedSendToken = nil
    fetchAssets()
  }

  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }

  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
