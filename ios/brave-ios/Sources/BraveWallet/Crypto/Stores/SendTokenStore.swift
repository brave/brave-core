// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import Combine
import Foundation
import Strings

/// A store contains data for sending tokens
public class SendTokenStore: ObservableObject, WalletObserverStore {
  /// User's visible asset with selected account and chain
  @Published var userVisibleAssets: [BraveWallet.BlockchainToken] = []
  /// The current selected token to send. Default with nil value.
  @Published var selectedSendToken: BraveWallet.BlockchainToken? {
    didSet {
      update()  // need to update `selectedSendTokenBalance` and `selectedSendNFTMetadata`
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
        withTimeInterval: 0.25,  // try not to validate for every character entered
        repeats: false,
        block: { [weak self] _ in
          self?.validateSendAddress()
        }
      )
    }
  }
  /// An error for input send address. Nil for no error.
  @Published var addressError: AddressError?
  /// The amount the user inputs to send
  @Published var sendAmount = "" {
    didSet {
      if oldValue != sendAmount {
        // only reset if changed (field focus re-assigns current value)
        isSendingMaxValue = false
      }
      sendAmountUpdatedTimer?.invalidate()
      sendAmountUpdatedTimer = Timer.scheduledTimer(
        withTimeInterval: 0.25,  // try not to validate for every character entered
        repeats: false,
        block: { [weak self] _ in
          self?.validateBalance()
        }
      )
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
  // The BTC balance types for the currently selected account. Key is `account.id`.
  @Published private(set) var btcBalances: [String: [BTCBalanceType: Double]] = [:]
  /// Price of BTC in the users selected currency.
  @Published private(set) var btcPrice: Double?

  /// Indicates if user tapped `100%` button for current `sendAmount`
  private var isSendingMaxValue: Bool = false

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
    case notFilAddress
    case notBtcAddress

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
        return String.localizedStringWithFormat(
          Strings.Wallet.sendErrorDomainNotRegistered,
          BraveWallet.CoinType.sol.localizedTitle
        )
      case .ensError:
        return String.localizedStringWithFormat(
          Strings.Wallet.sendErrorDomainNotRegistered,
          BraveWallet.CoinType.eth.localizedTitle
        )
      case .udError:
        return String.localizedStringWithFormat(
          Strings.Wallet.sendErrorDomainNotRegistered,
          BraveWallet.CoinType.eth.localizedTitle
        )
      case .notFilAddress:
        return Strings.Wallet.sendErrorInvalidRecipientAddress
      case .notBtcAddress:
        return Strings.Wallet.sendErrorBtcAddressNotValid
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

  private(set) lazy var selectTokenStore = SelectAccountTokenStore(
    didSelect: { [weak self] account, token in
      self?.didSelect(account: account, token: token)
    },
    keyringService: keyringService,
    rpcService: rpcService,
    walletService: walletService,
    assetRatioService: assetRatioService,
    bitcoinWalletService: bitcoinWalletService,
    ipfsApi: ipfsApi,
    userAssetManager: assetManager,
    query: prefilledToken?.symbol
  )

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let assetRatioService: BraveWalletAssetRatioService
  private let ethTxManagerProxy: BraveWalletEthTxManagerProxy
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
  private var allTokens: [BraveWallet.BlockchainToken] = []
  private var sendAddressUpdatedTimer: Timer?
  private var sendAmountUpdatedTimer: Timer?
  private var prefilledToken: BraveWallet.BlockchainToken?
  private var metadataCache: [String: NFTMetadata] = [:]
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType
  private var keyringServiceObserver: KeyringServiceObserver?
  private var rpcServiceObserver: JsonRpcServiceObserver?

  var isObserving: Bool {
    keyringServiceObserver != nil && rpcServiceObserver != nil
  }

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    assetRatioService: BraveWalletAssetRatioService,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    bitcoinWalletService: BraveWalletBitcoinWalletService,
    prefilledToken: BraveWallet.BlockchainToken?,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.assetRatioService = assetRatioService
    self.ethTxManagerProxy = ethTxManagerProxy
    self.solTxManagerProxy = solTxManagerProxy
    self.bitcoinWalletService = bitcoinWalletService
    self.prefilledToken = prefilledToken
    self.ipfsApi = ipfsApi
    self.assetManager = userAssetManager

    self.setupObservers()
  }

  func tearDown() {
    keyringServiceObserver = nil
    rpcServiceObserver = nil

    selectTokenStore.tearDown()
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.assetManager.addUserAssetDataObserver(self)
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _selectedWalletAccountChanged: { [weak self] _ in
        guard let self else { return }
        self.selectedSendTokenBalance = nil
        self.addressError = nil
        self.update()  // `selectedSendTokenBalance` needs updated for new account
        self.validateSendAddress()  // `sendAddress` may equal selected account address
      }
    )
    self.rpcServiceObserver = JsonRpcServiceObserver(
      rpcService: rpcService,
      _chainChangedEvent: { [weak self] _, coin, _ in
        guard let self else { return }
        self.selectedSendToken = nil
        self.selectedSendTokenBalance = nil
        if coin != .eth {  // if changed to ethereum coin network, address is still valid
          // offchain resolve is for ENS-only, for other coin types the address is invalid
          self.isOffchainResolveRequired = false
        }
        // `selectedSendToken` & `selectedSendTokenBalance` need updated for new chain
        self.update()
        self.validateSendAddress()  // `sendAddress` may no longer be valid if coin type changed
      }
    )

    self.selectTokenStore.setupObservers()
  }

  func suggestedAmountTapped(_ amount: ShortcutAmountGrid.Amount) {
    var decimalPoint = 6
    var rounded = true
    if amount == .all {
      decimalPoint = Int(selectedSendToken?.decimals ?? 18)
      rounded = false
    }
    sendAmount =
      ((selectedSendTokenBalance ?? 0) * amount.rawValue).decimalExpansion(
        precisionAfterDecimalPoint: decimalPoint,
        rounded: rounded
      ).trimmingTrailingZeros
    // `isSendingMaxValue` reset to false in `sendAmount` didSet.
    // Assign true after we've updated the value via 100% button.
    isSendingMaxValue = amount == .all
  }

  func didSelect(account: BraveWallet.AccountInfo, token: BraveWallet.BlockchainToken) {
    Task { @MainActor in
      guard let selectedAccount = await keyringService.allAccounts().selectedAccount else {
        assertionFailure("selectedAccount should never be nil.")
        return
      }
      if selectedAccount.accountId.uniqueKey != account.accountId.uniqueKey {
        _ = await self.keyringService.setSelectedAccount(accountId: account.accountId)
      }

      let selectedChain = await rpcService.network(coin: selectedAccount.coin, origin: nil)
      if self.selectedSendToken != token || selectedChain.chainId != token.chainId {
        _ = await self.rpcService.setNetwork(chainId: token.chainId, coin: token.coin, origin: nil)
        self.prefilledToken = token
      }

      self.update()
    }
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
      let allNetworksForTokenCoin = await rpcService.allNetworks().filter({
        $0.coin == prefilledToken.coin
      })
      guard
        let networkForToken = allNetworksForTokenCoin.first(where: {
          $0.chainId == prefilledToken.chainId
        })
      else {
        // don't set prefilled token if it belongs to a network we don't know
        return
      }
      let success = await rpcService.setNetwork(
        chainId: networkForToken.chainId,
        coin: networkForToken.coin,
        origin: nil
      )
      if success {
        let accountId = await walletService.ensureSelectedAccountForChain(
          coin: networkForToken.coin,
          chainId: networkForToken.chainId
        )
        if accountId == nil {
          assertionFailure(
            "accountId should not be nil. An account should exist for token's chain."
          )
        }
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
      guard let selectedAccount = await keyringService.allAccounts().selectedAccount else {
        assertionFailure("selectedAccount should never be nil.")
        return
      }
      var network = await rpcService.network(coin: selectedAccount.coin, origin: nil)
      await validatePrefilledToken(on: &network)  // network may change
      // fetch user visible assets
      let userVisibleAssets = await assetManager.getUserAssets(
        networks: [network],
        visible: true
      ).flatMap { $0.tokens }
      let allTokens = await self.blockchainRegistry.allTokens(
        chainId: network.chainId,
        coin: network.coin
      )
      guard !Task.isCancelled else { return }
      if selectedSendToken == nil {
        self.selectedSendToken =
          userVisibleAssets.first(where: { network.isNativeAsset($0) })
          ?? userVisibleAssets.first(where: {
            $0.symbol.caseInsensitiveCompare("bat") == .orderedSame
          })
          ?? userVisibleAssets.sorted(by: { $0.name < $1.name }).first
      }
      self.userVisibleAssets = userVisibleAssets
      self.allTokens = allTokens
      self.validateSendAddress()  // `sendAddress` may match a token contract address
      // fetch balance for `selectedSendToken`
      guard let selectedSendToken = self.selectedSendToken else {
        self.selectedSendTokenBalance = nil  // no selected account, or send token is nil
        return
      }
      let balance: BDouble?
      if selectedSendToken.coin == .btc {
        let btcBalances = await bitcoinWalletService.fetchBTCBalances(
          accountId: selectedAccount.accountId
        )
        balance = BDouble(btcBalances[.available, default: 0])
        if self.btcPrice == nil,
          btcBalances[.pending] != 0,  // price needed for details display
          let btcPriceString = await assetRatioService.fetchPrices(
            for: [selectedSendToken.assetRatioId],
            toAssets: [selectTokenStore.currencyCode],
            timeframe: .oneDay
          )[selectedSendToken.assetRatioId],
          let btcPriceDouble = Double(btcPriceString)
        {
          self.btcPrice = btcPriceDouble
        }
        self.btcBalances[selectedAccount.id] = btcBalances
      } else {
        if let assetBalance = self.assetManager.getAssetBalances(
          for: selectedSendToken,
          account: selectedAccount.id
        )?.first {
          balance = BDouble(assetBalance.balance)
        } else {
          balance = await self.rpcService.balance(
            for: selectedSendToken,
            in: selectedAccount.address,
            network: network,
            decimalFormatStyle: .decimals(precision: Int(selectedSendToken.decimals))
          )
        }
      }

      if selectedSendToken.isErc721 || selectedSendToken.isNft,
        metadataCache[selectedSendToken.id] == nil
      {
        metadataCache[selectedSendToken.id] = await rpcService.fetchNFTMetadata(
          for: selectedSendToken,
          ipfsApi: self.ipfsApi
        )
      }
      guard !Task.isCancelled else { return }
      self.selectedSendTokenBalance = balance
      self.selectedSendNFTMetadata = metadataCache[selectedSendToken.id]
      self.validateBalance()
    }
  }

  private var validateSendAddressTask: Task<Void, Never>?
  private func validateSendAddress() {
    validateSendAddressTask?.cancel()
    validateSendAddressTask = Task { @MainActor in
      guard !sendAddress.isEmpty,
        let selectedAccount = await keyringService.allAccounts().selectedAccount,
        !Task.isCancelled
      else {
        return
      }
      switch selectedAccount.coin {
      case .eth:
        await validateEthereumSendAddress(fromAddress: selectedAccount.address)
      case .sol:
        await validateSolanaSendAddress(fromAddress: selectedAccount.address)
      case .fil:
        validateFilcoinSendAddress()
      case .btc:
        validateBitcoinSendAddress(fromAccount: selectedAccount)
      case .zec:
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
      let (address, isOffchainConsentRequired, status, _) = await rpcService.ensGetEthAddr(
        domain: domain
      )
      guard !Task.isCancelled else { return }
      if isOffchainConsentRequired {
        self.isOffchainResolveRequired = true
        self.addressError = nil
        return  // do not continue unless ens is enabled
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
      } else if (userVisibleAssets.first(where: {
        $0.contractAddress.lowercased() == normalizedToAddress
      }) != nil)
        || (allTokens.first(where: { $0.contractAddress.lowercased() == normalizedToAddress })
          != nil)
      {
        // 3. check if send address is a contract address
        addressError = .contractAddress
      } else {
        let checksumAddress = await keyringService.checksumEthAddress(sendAddress)
        if sendAddress == checksumAddress {
          // 4. check if send address is the same as the checksum address from the `KeyringService`
          addressError = nil
        } else if sendAddress.removingHexPrefix.lowercased() == sendAddress.removingHexPrefix
          || sendAddress.removingHexPrefix.uppercased() == sendAddress.removingHexPrefix
        {
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
    let (address, status, _) = await rpcService.unstoppableDomainsGetWalletAddr(
      domain: domain,
      token: token
    )
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

  private func validateFilcoinSendAddress() {
    addressError = sendAddress.isFILAddress ? nil : .notFilAddress
  }

  private func validateBitcoinSendAddress(fromAccount: BraveWallet.AccountInfo) {
    guard let selectedSendToken else { return }
    let isMainnet = selectedSendToken.chainId == BraveWallet.BitcoinMainnet
    addressError = sendAddress.isBTCAddress(isMainnet: isMainnet) ? nil : .notBtcAddress
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
      let (address, status, _) = await rpcService.snsGetSolAddr(domain: domain)
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
    } else {  // not supported SNS extension, validate address
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
      case let sendAmount = (selectedSendToken.isErc721 || selectedSendToken.isNft)
        ? "1" : self.sendAmount,
      let sendAmount = BDouble(sendAmount)
    else {
      sendError = nil
      return
    }
    sendError = balance < sendAmount ? .insufficientBalance : nil
  }

  func sendToken(
    amount: String,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    self.isMakingTx = true
    guard let token = self.selectedSendToken else {
      self.isMakingTx = false
      completion(false, Strings.Wallet.internalErrorMessage)
      return
    }
    let amount = (token.isErc721 || token.isNft) ? "1" : amount
    keyringService.allAccounts { allAccounts in
      guard let selectedAccount = allAccounts.selectedAccount else {
        self.isMakingTx = false
        completion(false, Strings.Wallet.internalErrorMessage)
        return
      }
      switch selectedAccount.coin {
      case .eth:
        self.sendTokenOnEth(
          amount: amount,
          token: token,
          from: selectedAccount,
          completion: completion
        )
      case .sol:
        self.sendTokenOnSol(
          amount: amount,
          token: token,
          from: selectedAccount,
          completion: completion
        )
      case .fil:
        self.sendTokenOnFil(
          amount: amount,
          token: token,
          from: selectedAccount.accountId,
          completion: completion
        )
      case .btc:
        self.sendTokenOnBtc(
          amount: amount,
          token: token,
          from: selectedAccount.accountId,
          completion: completion
        )
      default:
        self.isMakingTx = false
        completion(false, Strings.Wallet.internalErrorMessage)
      }
    }
  }

  func sendTokenOnEth(
    amount: String,
    token: BraveWallet.BlockchainToken,
    from fromAccountInfo: BraveWallet.AccountInfo,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    isMakingTx = true
    let walletAmountFormatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))
    guard
      let weiHexString = walletAmountFormatter.weiString(
        from: amount.normalizedDecimals,
        radix: .hex,
        decimals: Int(token.decimals)
      )
    else {
      isMakingTx = false
      completion(false, Strings.Wallet.internalErrorMessage)
      return
    }

    let sendToAddress = resolvedAddress ?? sendAddress
    rpcService.network(coin: .eth, origin: nil) { [weak self] network in
      guard let self = self else { return }
      if network.isNativeAsset(token) {
        let params = BraveWallet.NewEvmTransactionParams(
          chainId: network.chainId,
          from: fromAccountInfo.accountId,
          to: sendToAddress,
          value: "0x\(weiHexString)",
          gasLimit: "",
          data: .init()
        )
        self.txService.addUnapprovedEvmTransaction(
          params: params
        ) { success, txMetaId, errorMessage in
          self.isMakingTx = false
          completion(success, errorMessage)
        }
      } else if token.isErc721 {
        self.ethTxManagerProxy.makeErc721TransferFromData(
          from: fromAccountInfo.address,
          to: sendToAddress,
          tokenId: token.tokenId,
          contractAddress: token.contractAddress
        ) { success, data in
          guard success else {
            completion(false, nil)
            return
          }
          let params = BraveWallet.NewEvmTransactionParams(
            chainId: network.chainId,
            from: fromAccountInfo.accountId,
            to: token.contractAddress,
            value: "0x0",
            gasLimit: "",
            data: data
          )
          self.txService.addUnapprovedEvmTransaction(
            params: params
          ) { success, txMetaId, errorMessage in
            self.isMakingTx = false
            completion(success, errorMessage)
          }
        }
      } else {
        self.ethTxManagerProxy.makeErc20TransferData(
          toAddress: sendToAddress,
          amount: "0x\(weiHexString)"
        ) {
          success,
          data in
          guard success else {
            completion(false, nil)
            return
          }

          let params = BraveWallet.NewEvmTransactionParams(
            chainId: network.chainId,
            from: fromAccountInfo.accountId,
            to: token.contractAddress,
            value: "0x0",
            gasLimit: "",
            data: data
          )
          self.txService.addUnapprovedEvmTransaction(
            params: params
          ) { success, txMetaId, errorMessage in
            self.isMakingTx = false
            completion(success, errorMessage)
          }
        }
      }
    }
  }

  private func sendTokenOnSol(
    amount: String,
    token: BraveWallet.BlockchainToken,
    from fromAccountInfo: BraveWallet.AccountInfo,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    isMakingTx = true
    guard
      let amount = WalletAmountFormatter.decimalToAmount(
        amount.normalizedDecimals,
        tokenDecimals: Int(token.decimals)
      )
    else {
      isMakingTx = false
      completion(false, Strings.Wallet.internalErrorMessage)
      return
    }

    let sendToAddress = resolvedAddress ?? sendAddress
    rpcService.network(coin: .sol, origin: nil) { [weak self] network in
      guard let self = self else { return }
      if network.isNativeAsset(token) {
        self.solTxManagerProxy.makeSystemProgramTransferTxData(
          from: fromAccountInfo.address,
          to: sendToAddress,
          lamports: amount
        ) { solTxData, error, errMsg in
          guard let solanaTxData = solTxData else {
            self.isMakingTx = false
            completion(false, errMsg)
            return
          }
          let txDataUnion = BraveWallet.TxDataUnion(solanaTxData: solanaTxData)
          self.txService.addUnapprovedTransaction(
            txDataUnion: txDataUnion,
            chainId: network.chainId,
            from: fromAccountInfo.accountId
          ) { success, txMetaId, errMsg in
            self.isMakingTx = false
            completion(success, errMsg)
          }
        }
      } else {
        self.solTxManagerProxy.makeTokenProgramTransferTxData(
          chainId: network.chainId,
          splTokenMintAddress: token.contractAddress,
          fromWalletAddress: fromAccountInfo.address,
          toWalletAddress: sendToAddress,
          amount: amount,
          decimals: UInt8(token.decimals)
        ) { solTxData, error, errMsg in
          guard let solanaTxData = solTxData else {
            self.isMakingTx = false
            completion(false, errMsg)
            return
          }
          let txDataUnion = BraveWallet.TxDataUnion(solanaTxData: solanaTxData)
          self.txService.addUnapprovedTransaction(
            txDataUnion: txDataUnion,
            chainId: network.chainId,
            from: fromAccountInfo.accountId
          ) { success, txMetaId, errorMessage in
            self.isMakingTx = false
            completion(success, errorMessage)
          }
        }
      }
    }
  }

  private func sendTokenOnFil(
    amount: String,
    token: BraveWallet.BlockchainToken,
    from fromAccountId: BraveWallet.AccountId,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    isMakingTx = true
    let walletAmountFormatter = WalletAmountFormatter(
      decimalFormatStyle: .decimals(precision: Int(token.decimals))
    )
    guard
      let weiString = walletAmountFormatter.weiString(
        from: amount.normalizedDecimals,
        decimals: Int(token.decimals)
      )
    else {
      isMakingTx = false
      completion(false, Strings.Wallet.internalErrorMessage)
      return
    }

    rpcService.network(coin: .fil, origin: nil) { [weak self] network in
      guard let self = self else { return }
      let filTxData = BraveWallet.FilTxData(
        nonce: "",
        gasPremium: "",
        gasFeeCap: "",
        gasLimit: "",
        maxFee: "0",
        to: sendAddress,
        value: weiString
      )
      self.txService.addUnapprovedTransaction(
        txDataUnion: BraveWallet.TxDataUnion(filTxData: filTxData),
        chainId: network.chainId,
        from: fromAccountId
      ) { success, txMetaId, errorMessage in
        self.isMakingTx = false
        completion(success, errorMessage)
      }
    }
  }

  private func sendTokenOnBtc(
    amount: String,
    token: BraveWallet.BlockchainToken,
    from fromAccountId: BraveWallet.AccountId,
    completion: @escaping (_ success: Bool, _ errMsg: String?) -> Void
  ) {
    isMakingTx = true
    guard
      let amountInSatoshi = WalletAmountFormatter.decimalToAmount(
        amount.normalizedDecimals,
        tokenDecimals: Int(token.decimals)
      )
    else {
      isMakingTx = false
      completion(false, Strings.Wallet.internalErrorMessage)
      return
    }
    let btcTxData: BraveWallet.BtcTxData = .init(
      to: sendAddress,
      amount: amountInSatoshi,
      sendingMaxAmount: isSendingMaxValue,
      fee: 0,
      inputs: [],
      outputs: []
    )
    self.txService.addUnapprovedTransaction(
      txDataUnion: BraveWallet.TxDataUnion(btcTxData: btcTxData),
      chainId: token.chainId,
      from: fromAccountId
    ) { success, txMetaId, errorMessage in
      self.isMakingTx = false
      completion(success, errorMessage)
    }
  }

  @MainActor func fetchNFTMetadata(
    tokens: [BraveWallet.BlockchainToken]
  ) async -> [String: NFTMetadata] {
    return await rpcService.fetchNFTMetadata(tokens: tokens, ipfsApi: ipfsApi)
  }
}

extension SendTokenStore: WalletUserAssetDataObserver {
  public func cachedBalanceRefreshed() {
    update()
  }

  public func userAssetUpdated() {
  }
}
