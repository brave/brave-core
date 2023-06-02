// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Strings
import BigNumber
import Combine

/// A store contains data for sending tokens
public class SendTokenStore: ObservableObject {
  /// User's asset with selected account and chain
  @Published var userAssets: [BraveWallet.BlockchainToken] = []
  /// The current selected token to send. Default with nil value.
  @Published var selectedSendToken: BraveWallet.BlockchainToken? {
    didSet {
      update() // need to update `selectedSendTokenBalance` and `selectedSendNFTMetadata`
      // Unstoppable Domains are resolved based on currently selected token.
      validateSendAddress()
    }
  }
  /// The current selected NFT metadata. Default with nil value.
  @Published var selectedSendNFTMetadata: NFTMetadata?
  /// The current selected token balance. Default with nil value.
  @Published var selectedSendTokenBalance: BDouble?
  /// A boolean indicates if this store is making an unapproved tx
  @Published var isMakingTx = false
  /// The destination account address
  @Published var sendAddress = "" {
    didSet {
      if oldValue != sendAddress {
        resolvedAddress = nil
        isOffchainResolveRequired = false
      }
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
  /// If we are currently resolving an SNS or ENS address
  @Published private(set) var isResolvingAddress: Bool = false
  /// The address returned from SNS / ENS
  @Published private(set) var resolvedAddress: String?
  /// If the current `sendAddress` needs to be resolved offchain
  @Published private(set) var isOffchainResolveRequired: Bool = false

  enum AddressError: LocalizedError, Equatable {
    case sameAsFromAddress
    case contractAddress
    case notEthAddress
    case missingChecksum
    case invalidChecksum
    case notSolAddress
    case snsError(domain: String)
    case ensError(domain: String)
    case udError(domain: String)

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
      case .snsError:
        return String.localizedStringWithFormat(Strings.Wallet.sendErrorDomainNotRegistered, BraveWallet.CoinType.sol.localizedTitle)
      case .ensError:
        return String.localizedStringWithFormat(Strings.Wallet.sendErrorDomainNotRegistered, BraveWallet.CoinType.eth.localizedTitle)
      case .udError:
        return String.localizedStringWithFormat(Strings.Wallet.sendErrorDomainNotRegistered, BraveWallet.CoinType.eth.localizedTitle)
      }
    }
    
    var shouldBlockSend: Bool {
      switch self {
      case .missingChecksum:
        return false
      default:
        return true
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
  private var prefilledToken: BraveWallet.BlockchainToken?
  private var metadataCache: [String: NFTMetadata] = [:]
  private let ipfsApi: IpfsAPI

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    prefilledToken: BraveWallet.BlockchainToken?,
    ipfsApi: IpfsAPI
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.ethTxManagerProxy = ethTxManagerProxy
    self.solTxManagerProxy = solTxManagerProxy
    self.prefilledToken = prefilledToken
    self.ipfsApi = ipfsApi

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
  
  @MainActor private func validatePrefilledToken(on network: inout BraveWallet.NetworkInfo) async {
    guard let prefilledToken = self.prefilledToken else {
      return
    }
    if prefilledToken.coin == network.coin && prefilledToken.chainId == network.chainId {
      // valid for current network
      self.selectedSendToken = prefilledToken
    } else {
      // need to try and select correct network.
      let allNetworksForTokenCoin = await rpcService.allNetworks(prefilledToken.coin)
      guard let networkForToken = allNetworksForTokenCoin.first(where: { $0.chainId == prefilledToken.chainId }) else {
        // don't set prefilled token if it belongs to a network we don't know
        return
      }
      let success = await rpcService.setNetwork(networkForToken.chainId, coin: networkForToken.coin, origin: nil)
      if success {
        self.selectedSendToken = prefilledToken
      }
    }
    self.prefilledToken = nil
  }

  /// Cancellable for the last running `update()` Task.
  private var updateTask: Task<(), Never>?
  /// Updates the `userAssets`, `allTokens`, and `selectedSendTokenBalance`
  func update() {
    self.updateTask?.cancel()
    self.updateTask = Task { @MainActor in
      self.isLoading = true
      defer { self.isLoading = false }
      var coin = await self.walletService.selectedCoin()
      var network = await self.rpcService.network(coin, origin: nil)
      await validatePrefilledToken(on: &network) // network may change
      coin = network.coin // in case network changed
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
  
      if (selectedSendToken.isErc721 || selectedSendToken.isNft), metadataCache[selectedSendToken.id] == nil {
        metadataCache[selectedSendToken.id] = await rpcService.fetchNFTMetadata(for: selectedSendToken, ipfsApi: self.ipfsApi)
      }
      guard !Task.isCancelled else { return }
      self.selectedSendTokenBalance = balance
      self.selectedSendNFTMetadata = metadataCache[selectedSendToken.id]
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
  
  private var validateSendAddressTask: Task<Void, Never>?
  private func validateSendAddress() {
    validateSendAddressTask?.cancel()
    validateSendAddressTask = Task { @MainActor in
      guard !sendAddress.isEmpty,
            case let coin = await self.walletService.selectedCoin(),
            let sendFromAddress = await self.keyringService.selectedAccount(coin),
            !Task.isCancelled else {
        return
      }
      switch coin {
      case .eth:
        await validateEthereumSendAddress(fromAddress: sendFromAddress)
      case .sol:
        await validateSolanaSendAddress(fromAddress: sendFromAddress)
      case .fil, .btc:
        break
      @unknown default:
        break
      }
    }
  }
  
  @MainActor private func validateEthereumSendAddress(fromAddress: String) async {
    let normalizedFromAddress = fromAddress.lowercased()
    let normalizedToAddress = sendAddress.lowercased()
    let isSupportedENSExtension = sendAddress.endsWithSupportedENSExtension
    let isSupportedUDExtension = sendAddress.endsWithSupportedUDExtension
    if isSupportedENSExtension {
      self.resolvedAddress = nil
      self.isResolvingAddress = true
      defer { self.isResolvingAddress = false }
      let domain = sendAddress
      let (address, isOffchainConsentRequired, status, _) = await rpcService.ensGetEthAddr(domain)
      guard !Task.isCancelled else { return }
      if isOffchainConsentRequired {
        self.isOffchainResolveRequired = true
        self.addressError = nil
        return // do not continue unless ens is enabled
      }
      if status != .success || address.isEmpty {
        addressError = .ensError(domain: sendAddress)
        return
      }
      // If found address is the same as the selectedAccounts Wallet Address
      if address.lowercased() == normalizedFromAddress {
        addressError = .sameAsFromAddress
        return
      }
      guard domain == sendAddress, !Task.isCancelled else {
        // address changed while resolving, or validation cancelled.
        return
      }
      // store address for sending
      resolvedAddress = address
      addressError = nil
    } else if isSupportedUDExtension {
      await resolveUnstoppableDomain(sendAddress)
    } else {
      if !sendAddress.isETHAddress {
        // 1. check if send address is a valid eth address
        addressError = .notEthAddress
      } else if normalizedFromAddress == normalizedToAddress {
        // 2. check if send address is the same as the from address
        addressError = .sameAsFromAddress
      } else if (userAssets.first(where: { $0.contractAddress.lowercased() == normalizedToAddress }) != nil)
                  || (allTokens.first(where: { $0.contractAddress.lowercased() == normalizedToAddress }) != nil) {
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
    }
  }
  
  @MainActor private func resolveUnstoppableDomain(_ domain: String) async {
    guard selectedSendToken != nil else {
      // token is required for `unstoppableDomainsGetWalletAddr`
      // else it returns `invalidParams` error immediately
      return
    }
    self.resolvedAddress = nil
    self.isResolvingAddress = true
    defer { self.isResolvingAddress = false }
    let token = selectedSendToken
    let (address, status, _) = await rpcService.unstoppableDomainsGetWalletAddr(domain, token: token)
    guard !Task.isCancelled else { return }
    if status != .success || address.isEmpty {
      addressError = .udError(domain: sendAddress)
      return
    }
    guard domain == sendAddress, token == selectedSendToken, !Task.isCancelled else {
      // address changed while resolving, or validation cancelled.
      return
    }
    // store address for sending
    resolvedAddress = address
    addressError = nil
  }
  
  public func enableENSOffchainLookup() {
    Task { @MainActor in
      rpcService.setEnsOffchainLookupResolveMethod(.enabled)
      self.isOffchainResolveRequired = false
      await self.validateEthereumSendAddress(fromAddress: sendAddress)
    }
  }
  
  @MainActor private func validateSolanaSendAddress(fromAddress: String) async {
    let normalizedFromAddress = fromAddress.lowercased()
    let normalizedToAddress = sendAddress.lowercased()
    let isSupportedSNSExtension = sendAddress.endsWithSupportedSNSExtension
    let isSupportedUDExtension = sendAddress.endsWithSupportedUDExtension
    if isSupportedSNSExtension {
      self.resolvedAddress = nil
      self.isResolvingAddress = true
      defer { self.isResolvingAddress = false }
      let domain = sendAddress
      let (address, status, _) = await rpcService.snsGetSolAddr(domain)
      guard !Task.isCancelled else { return }
      if status != .success || address.isEmpty {
        addressError = .snsError(domain: sendAddress)
        return
      }
      // If found address is the same as the selectedAccounts Wallet Address
      if address.lowercased() == normalizedFromAddress {
        addressError = .sameAsFromAddress
        return
      }
      guard domain == sendAddress, !Task.isCancelled else {
        // address changed while resolving, or validation cancelled.
        return
      }
      // store address for sending
      resolvedAddress = address
      addressError = nil
    } else if isSupportedUDExtension {
      await resolveUnstoppableDomain(sendAddress)
    } else { // not supported SNS extension, validate address
      let isValid = await walletService.isBase58EncodedSolanaPubkey(sendAddress)
      if !isValid {
        addressError = .notSolAddress
      } else if normalizedFromAddress == normalizedToAddress {
        addressError = .sameAsFromAddress
      } else {
        addressError = nil
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
    
    let sendToAddress = resolvedAddress ?? sendAddress

    isMakingTx = true
    rpcService.network(.eth, origin: nil) { [weak self] network in
      guard let self = self else { return }
      if network.isNativeAsset(token) {
        let baseData = BraveWallet.TxData(nonce: "", gasPrice: "", gasLimit: "", to: sendToAddress, value: "0x\(weiHexString)", data: .init(), signOnly: false, signedTransaction: nil)
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
        self.ethTxManagerProxy.makeErc721Transfer(fromData: fromAddress, to: sendToAddress, tokenId: token.tokenId, contractAddress: token.contractAddress) { success, data in
          guard success else {
            completion(false, nil)
            return
          }
          let baseData = BraveWallet.TxData(nonce: "", gasPrice: "", gasLimit: "", to: token.contractAddress, value: "0x0", data: data, signOnly: false, signedTransaction: nil)
          let txDataUnion = BraveWallet.TxDataUnion(ethTxData: baseData)
          self.txService.addUnapprovedTransaction(txDataUnion, from: fromAddress, origin: nil, groupId: nil) { success, txMetaId, errorMessage in
            self.isMakingTx = false
            completion(success, errorMessage)
          }
        }
      } else {
        self.ethTxManagerProxy.makeErc20TransferData(sendToAddress, amount: "0x\(weiHexString)") { success, data in
          guard success else {
            completion(false, nil)
            return
          }
          let baseData = BraveWallet.TxData(nonce: "", gasPrice: "", gasLimit: "", to: token.contractAddress, value: "0x0", data: data, signOnly: false, signedTransaction: nil)
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
    
    let sendToAddress = resolvedAddress ?? sendAddress

    rpcService.network(.sol, origin: nil) { [weak self] network in
      guard let self = self else { return }
      if network.isNativeAsset(token) {
        self.solTxManagerProxy.makeSystemProgramTransferTxData(
          fromAddress,
          to: sendToAddress,
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
          network.chainId,
          splTokenMintAddress: token.contractAddress,
          fromWalletAddress: fromAddress,
          toWalletAddress: sendToAddress,
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
  
  @MainActor func fetchNFTMetadata(tokens: [BraveWallet.BlockchainToken]) async -> [String: NFTMetadata] {
    return await rpcService.fetchNFTMetadata(tokens: tokens, ipfsApi: ipfsApi)
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
  
  public func accountsAdded(_ addedAccounts: [BraveWallet.AccountInfo]) {
  }
}

extension SendTokenStore: BraveWalletJsonRpcServiceObserver {
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType, origin: URLOrigin?) {
    selectedSendToken = nil
    selectedSendTokenBalance = nil
    if coin != .eth { // if changed to ethereum coin network, address is still valid
      // offchain resolve is for ENS-only, for other coin types the address is invalid
      isOffchainResolveRequired = false
    }
    update() // `selectedSendToken` & `selectedSendTokenBalance` need updated for new chain
    validateSendAddress() // `sendAddress` may no longer be valid if coin type changed
  }

  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }

  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
