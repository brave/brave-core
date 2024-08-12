// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import Combine
import Foundation
import Strings

/// A store contains data for swap tokens
public class SwapTokenStore: ObservableObject, WalletObserverStore {
  /// All  tokens for searching use
  @Published var allTokens: [BraveWallet.BlockchainToken] = []
  /// The current selected token to swap from. Default with nil value.
  @Published var selectedFromToken: BraveWallet.BlockchainToken? {
    didSet {
      if oldValue != selectedFromToken {
        clearAllAmount()
      }
      if let token = selectedFromToken {
        fetchTokenBalance(for: token) { [weak self] balance in
          self?.selectedFromTokenBalance = balance
        }
      }
    }
  }
  /// The current selected token to swap to. Default with nil value
  @Published var selectedToToken: BraveWallet.BlockchainToken? {
    didSet {
      if oldValue != selectedToToken {
        clearAllAmount()
      }
      if let token = selectedToToken {
        fetchTokenBalance(for: token) { [weak self] balance in
          self?.selectedToTokenBalance = balance
        }
      }
    }
  }
  /// The current selected token balance to swap from. Default with nil value.
  @Published var selectedFromTokenBalance: BDouble?
  /// The current selected token balance to swap to. Default with nil value.
  @Published var selectedToTokenBalance: BDouble?
  /// The current market price for selected token to swap from.
  @Published var selectedFromTokenPrice = "0"
  /// The state of swap screen
  @Published var state: SwapState = .idle {
    didSet {
      if case .error = state {
        isMakingTx = false
      }
    }
  }
  /// The sell amount in this swap
  @Published var sellAmount = "" {
    didSet {
      if sellAmount != oldValue {
        // price quote requested for a different amount
        priceQuoteTask?.cancel()
      }
      guard !sellAmount.isEmpty, BDouble(sellAmount.normalizedDecimals) != nil else {
        state = .idle
        return
      }
      if oldValue != sellAmount && !isUpdatingPriceQuote && !isMakingTx {
        timer?.invalidate()
        timer = Timer.scheduledTimer(
          withTimeInterval: 0.25,
          repeats: false,
          block: { [weak self] _ in
            self?.fetchPriceQuote(base: .perSellAsset)
          }
        )
      }
    }
  }
  /// The buy amount in this swap
  @Published var buyAmount = "" {
    didSet {
      if buyAmount != oldValue {
        // price quote requested for a different amount
        priceQuoteTask?.cancel()
      }
      guard !buyAmount.isEmpty, BDouble(buyAmount.normalizedDecimals) != nil else {
        state = .idle
        return
      }
      if oldValue != buyAmount && !isUpdatingPriceQuote && !isMakingTx {
        timer?.invalidate()
        timer = Timer.scheduledTimer(
          withTimeInterval: 0.25,
          repeats: false,
          block: { [weak self] _ in
            self?.fetchPriceQuote(base: .perBuyAsset)
          }
        )
      }
    }
  }
  /// The latest slippage option that user selected
  @Published var slippageOption = SlippageGrid.Option.halfPercent {
    didSet {
      slippage = slippageOption.value
    }
  }
  /// The custom user input slippage percentage value which will override `slippageOption` if it is not nil
  @Published var overrideSlippage: Int? {
    didSet {
      if let overrideSlippage = overrideSlippage {
        slippage = Double(overrideSlippage) / 100.0
      } else {
        slippage = slippageOption.value
      }
    }
  }
  /// A boolean indicating if this store is making an unapproved tx
  @Published var isMakingTx = false
  /// A boolean indicating if this store is fetching updated price quote
  @Published var isUpdatingPriceQuote = false

  struct CurrentSwapQuoteInfo {
    /// If the quote was fetched based on sell/from amount or buy/to amount.
    /// This needs stored so we can generate the Eth Swap Tx using the same
    /// values as we fetched the quote for (support `exactOut` mode with 0x).
    let base: SwapParamsBase
    /// The current swap price quote
    let swapQuote: BraveWallet.SwapQuoteUnion?
    /// The swap fees for the current price quote
    let swapFees: BraveWallet.SwapFees?
  }
  /// The details for the current swap quote on display
  @Published var currentSwapQuoteInfo: CurrentSwapQuoteInfo?

  /// If the Brave Fee is voided for this swap.
  var isBraveFeeVoided: Bool {
    guard let swapFees = currentSwapQuoteInfo?.swapFees else { return false }
    return swapFees.discountCode != .none && !swapFees.hasBraveFee
  }

  /// The brave fee percentage for this swap.
  /// When `isBraveFeeVoided`, will return the fee being voided (so Free can be displayed beside % value voided)
  var braveFeeForDisplay: String? {
    guard let swapFees = currentSwapQuoteInfo?.swapFees else { return nil }
    let fee: String
    if swapFees.discountCode == .none {
      fee = swapFees.effectiveFeePct
    } else {
      if swapFees.hasBraveFee {
        fee = swapFees.effectiveFeePct
      } else {
        // Display as `Free ~braveFee.feePct%~`
        fee = swapFees.feePct
      }
    }
    return String(format: "%@%%", fee.trimmingTrailingZeros)
  }

  private let keyringService: BraveWalletKeyringService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let rpcService: BraveWalletJsonRpcService
  private let swapService: BraveWalletSwapService
  private let txService: BraveWalletTxService
  private let walletService: BraveWalletBraveWalletService
  private let ethTxManagerProxy: BraveWalletEthTxManagerProxy
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private let assetManager: WalletUserAssetManagerType
  private var accountInfo: BraveWallet.AccountInfo?
  private var slippage = 0.005 {
    didSet {
      timer?.invalidate()
      timer = Timer.scheduledTimer(
        withTimeInterval: 0.25,
        repeats: false,
        block: { [weak self] _ in
          self?.fetchPriceQuote(base: .perSellAsset)
        }
      )
    }
  }
  private var timer: Timer?
  private let batSymbol = "BAT"
  private let daiSymbol = "DAI"
  private let usdcSymbol = "USDC"
  private var prefilledToken: BraveWallet.BlockchainToken?
  private var keyringServiceObserver: KeyringServiceObserver?
  private var rpcServiceObserver: JsonRpcServiceObserver?

  var isObserving: Bool {
    keyringServiceObserver != nil && rpcServiceObserver != nil
  }

  enum SwapParamsBase {
    // calculating based on sell asset amount
    case perSellAsset
    // calculating based on buy asset amount
    case perBuyAsset
  }

  enum SwapState: Equatable {
    // when there is an error occurs. associated with an error message
    case error(String)
    // when erc20 token's allowance is less then sell amount. associated with a spender address
    // the future of this state would get erc20 token apporve data then create
    // an unapproved transaction
    case lowAllowance(String)
    // when able to swap
    case swap
    // has not passed validation
    case idle
  }

  public init(
    keyringService: BraveWalletKeyringService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    rpcService: BraveWalletJsonRpcService,
    swapService: BraveWalletSwapService,
    txService: BraveWalletTxService,
    walletService: BraveWalletBraveWalletService,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    userAssetManager: WalletUserAssetManagerType,
    prefilledToken: BraveWallet.BlockchainToken?
  ) {
    self.keyringService = keyringService
    self.blockchainRegistry = blockchainRegistry
    self.rpcService = rpcService
    self.swapService = swapService
    self.txService = txService
    self.walletService = walletService
    self.ethTxManagerProxy = ethTxManagerProxy
    self.solTxManagerProxy = solTxManagerProxy
    self.assetManager = userAssetManager
    self.prefilledToken = prefilledToken

    self.setupObservers()
  }

  func tearDown() {
    keyringServiceObserver = nil
    rpcServiceObserver = nil
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.assetManager.addUserAssetDataObserver(self)
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _selectedWalletAccountChanged: { [weak self] account in
        Task { @MainActor [self] in
          guard let network = await self?.rpcService.network(coin: account.coin, origin: nil),
            let isSwapSupported = await self?.swapService.isSwapSupported(chainId: network.chainId),
            isSwapSupported
          else {
            self?.accountInfo = account
            return
          }

          self?.selectedFromToken = nil
          self?.selectedToToken = nil
          self?.prepare(with: account) {
            self?.fetchPriceQuote(base: .perSellAsset)
          }
        }
      }
    )
    self.rpcServiceObserver = JsonRpcServiceObserver(
      rpcService: rpcService,
      _chainChangedEvent: { [weak self] chainId, coin, origin in
        Task { @MainActor [self] in
          guard let isSwapSupported = await self?.swapService.isSwapSupported(chainId: chainId),
            isSwapSupported
          else { return }
          guard
            let _ = await self?.walletService.ensureSelectedAccountForChain(
              coin: coin,
              chainId: chainId
            ),
            let selectedAccount = await self?.keyringService.allAccounts().selectedAccount
          else {
            assertionFailure("selectedAccount should never be nil.")
            return
          }

          self?.selectedFromToken = nil
          self?.selectedToToken = nil
          self?.prepare(with: selectedAccount) {
            self?.fetchPriceQuote(base: .perSellAsset)
          }
        }
      }
    )
  }

  private func fetchTokenBalance(
    for token: BraveWallet.BlockchainToken,
    completion: @escaping (_ balance: BDouble?) -> Void
  ) {
    guard let account = accountInfo else {
      completion(nil)
      return
    }

    rpcService.network(coin: token.coin, origin: nil) { [weak self] network in
      if let assetBalance = self?.assetManager.getAssetBalances(
        for: token,
        account: account.id
      )?.first(where: { $0.chainId == network.chainId }) {
        completion(BDouble(assetBalance.balance))
      } else {
        self?.rpcService.balance(
          for: token,
          in: account.address,
          network: network,
          decimalFormatStyle: .decimals(precision: Int(token.decimals))
        ) { balance in
          completion(balance)
        }
      }
    }
  }

  private func swapQuoteParameters(
    for base: SwapParamsBase,
    in network: BraveWallet.NetworkInfo
  ) -> BraveWallet.SwapQuoteParams? {
    guard
      let accountInfo = accountInfo,
      let sellToken = selectedFromToken,
      let buyToken = selectedToToken
    else { return nil }

    let walletAmountFormatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))
    let sellAddress = sellToken.contractAddress(in: network)
    let buyAddress = buyToken.contractAddress(in: network)
    let sellAmountInWei: String
    let buyAmountInWei: String
    switch base {
    case .perSellAsset:
      // make sure the base value should not be zero, otherwise, it will always return insufficient liquidity error.
      // following desktop to make a idle state
      if let sellAmountValue = BDouble(sellAmount.normalizedDecimals), sellAmountValue == 0 {
        return nil
      }
      sellAmountInWei =
        walletAmountFormatter.weiString(
          from: sellAmount.normalizedDecimals,
          radix: .decimal,
          decimals: Int(sellToken.decimals)
        ) ?? "0"
      buyAmountInWei = ""
    case .perBuyAsset:
      // same as sell amount. make sure base value should not be zero
      if let buyAmountValue = BDouble(buyAmount.normalizedDecimals), buyAmountValue == 0 {
        return nil
      }
      sellAmountInWei = ""
      buyAmountInWei =
        walletAmountFormatter.weiString(
          from: buyAmount.normalizedDecimals,
          radix: .decimal,
          decimals: Int(buyToken.decimals)
        ) ?? "0"
    }
    // We store 0.5% as 0.005, so multiply by 100 to get 0.5
    let slippagePercentage = slippage * 100
    let swapQuoteParams = BraveWallet.SwapQuoteParams(
      from: accountInfo.accountId,
      fromChainId: network.chainId,
      fromToken: sellAddress,
      fromAmount: sellAmountInWei,
      to: accountInfo.accountId,
      toChainId: network.chainId,
      toToken: buyAddress,
      toAmount: buyAmountInWei,
      slippagePercentage: "\(slippagePercentage)",
      routePriority: .cheapest,
      // TODO(stephenheaps): Enable Lifi support
      // https://github.com/brave/brave-browser/issues/39043
      provider: sellToken.coin == .sol ? .jupiter : .zeroEx
    )

    return swapQuoteParams
  }

  private func clearAllAmount() {
    sellAmount = ""
    buyAmount = ""
    selectedFromTokenPrice = "0"
  }

  @MainActor private func createEthSwapTransaction() async -> Bool {
    guard let currentSwapQuoteInfo, let accountInfo else {
      self.state = .error(Strings.Wallet.unknownError)
      self.clearAllAmount()
      return false
    }
    self.isMakingTx = true
    defer { self.isMakingTx = false }
    let coin = accountInfo.coin
    let network = await rpcService.network(coin: coin, origin: nil)
    guard
      let swapQuoteParams = self.swapQuoteParameters(for: currentSwapQuoteInfo.base, in: network)
    else {
      self.state = .error(Strings.Wallet.unknownError)
      self.clearAllAmount()
      return false
    }
    var gasLimit: String?
    var to: String?
    var value: String?
    var data: [NSNumber]?
    let walletAmountFormatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))

    if currentSwapQuoteInfo.swapQuote?.zeroExQuote != nil {
      let (swapTransactionUnion, _, _) = await swapService.transaction(
        params: .init(zeroExTransactionParams: swapQuoteParams)
      )
      guard let zeroExQuote = swapTransactionUnion?.zeroExTransaction else {
        self.state = .error(Strings.Wallet.unknownError)
        self.clearAllAmount()
        return false
      }
      // these values are already in wei
      gasLimit =
        "0x\(walletAmountFormatter.weiString(from: zeroExQuote.estimatedGas, radix: .hex, decimals: 0) ?? "0")"
      to = zeroExQuote.to
      value =
        "0x\(walletAmountFormatter.weiString(from: zeroExQuote.value, radix: .hex, decimals: 0) ?? "0")"
      data = .init(hexString: zeroExQuote.data) ?? .init()

    } else if let lifiQuote = currentSwapQuoteInfo.swapQuote?.lifiQuote {
      guard let route = lifiQuote.routes.first, let step = route.steps.first else {
        self.state = .error(Strings.Wallet.unknownError)
        self.clearAllAmount()
        return false
      }
      let (swapTransactionUnion, _, _) = await swapService.transaction(
        params: .init(lifiTransactionParams: step)
      )
      guard let lifiTransaction = swapTransactionUnion?.lifiTransaction,
        let evmTransaction = lifiTransaction.evmTransaction
      else {
        self.state = .error(Strings.Wallet.unknownError)
        self.clearAllAmount()
        return false
      }
      // these values are already in wei
      gasLimit =
        "0x\(walletAmountFormatter.weiString(from: evmTransaction.gasLimit, radix: .hex, decimals: 0) ?? "0")"
      to = evmTransaction.to
      value =
        "0x\(walletAmountFormatter.weiString(from: evmTransaction.value, radix: .hex, decimals: 0) ?? "0")"
      data = .init(hexString: evmTransaction.data) ?? .init()
    } else {
      assertionFailure("Only ZeroEx and LiFi supported for Ethereum swaps")
      self.state = .error(Strings.Wallet.unknownError)
      self.clearAllAmount()
      return false
    }

    guard let gasLimit,
      let to,
      let value,
      let data
    else {
      self.state = .error(Strings.Wallet.unknownError)
      self.clearAllAmount()
      return false
    }
    let params = BraveWallet.NewEvmTransactionParams(
      chainId: network.chainId,
      from: accountInfo.accountId,
      to: to,
      value: value,
      gasLimit: gasLimit,
      data: data
    )
    let (success, _, _) = await txService.addUnapprovedEvmTransaction(
      params: params
    )
    if !success {
      self.state = .error(Strings.Wallet.unknownError)
      self.clearAllAmount()
    }
    return success
  }

  @MainActor private func createERC20ApprovalTransaction(
    _ spenderAddress: String
  ) async -> Bool {
    guard
      let fromToken = selectedFromToken,
      let accountInfo = accountInfo
    else {
      return false
    }

    // IMPORTANT SECURITY NOTICE
    //
    // The token allowance suggested by Swap is always unlimited,
    // i.e., max(uint256). While unlimited approvals are not safe from a
    // security standpoint, and this puts the entire token balance at risk
    // if 0x contracts are ever exploited, we still opted for this to give
    // users a frictionless UX and save on gas fees.
    //
    // The transaction confirmation screen for ERC20 approve() shows a loud
    // security notice, and still allows users to edit the default approval
    // amount.
    let allowance = WalletConstants.maxUInt256
    self.isMakingTx = true
    defer { self.isMakingTx = false }
    let network = await rpcService.network(coin: accountInfo.coin, origin: nil)
    let (success, data) = await ethTxManagerProxy.makeErc20ApproveData(
      spenderAddress: spenderAddress,
      amount: allowance
    )
    guard success else {
      return false
    }
    let params = BraveWallet.NewEvmTransactionParams(
      chainId: network.chainId,
      from: accountInfo.accountId,
      to: fromToken.contractAddress(in: network),
      value: "0x0",
      gasLimit: "",
      data: data
    )
    let (addSuccess, _, _) = await txService.addUnapprovedEvmTransaction(
      params: params
    )
    if !addSuccess {
      self.state = .error(Strings.Wallet.unknownError)
      self.clearAllAmount()
    }
    return addSuccess
  }

  private func bumpFeeByOneGWei(with value: String) -> String? {
    guard let bv = BDouble(value, radix: 16) else { return nil }
    let bumpedValue = bv + (BDouble(10) ** 9)
    return bumpedValue.rounded().asString(radix: 16)
  }

  @MainActor private func checkAllowance(
    network: BraveWallet.NetworkInfo,
    ownerAddress: String,
    spenderAddress: String,
    amountToSend: BDouble,
    fromToken: BraveWallet.BlockchainToken
  ) async {
    let (allowance, status, _) = await rpcService.erc20TokenAllowance(
      contract: fromToken.contractAddress(in: network),
      ownerAddress: ownerAddress,
      spenderAddress: spenderAddress,
      chainId: network.chainId
    )
    let walletAmountFormatter = WalletAmountFormatter(
      decimalFormatStyle: .decimals(precision: Int(fromToken.decimals))
    )
    let allowanceValue =
      BDouble(
        walletAmountFormatter.decimalString(
          for: allowance.removingHexPrefix,
          radix: .hex,
          decimals: Int(fromToken.decimals)
        ) ?? ""
      ) ?? 0
    guard status == .success, amountToSend > allowanceValue else {
      // no problem with its allowance
      self.state = .swap
      return
    }
    self.state = .lowAllowance(spenderAddress)
  }

  // MARK: Price Quotes

  @MainActor private func fetchEthPriceQuote(
    base: SwapParamsBase,
    swapQuoteParams: BraveWallet.SwapQuoteParams
  ) async {
    self.isUpdatingPriceQuote = true
    defer { self.isUpdatingPriceQuote = false }
    let (swapQuoteUnion, swapFees, swapQuoteErrorUnion, _) = await swapService.quote(
      params: swapQuoteParams
    )
    guard !Task.isCancelled else { return }
    self.currentSwapQuoteInfo = .init(
      base: base,
      swapQuote: swapQuoteUnion,
      swapFees: swapFees
    )
    if let swapQuoteUnion = swapQuoteUnion {
      await self.handleSwapQuote(base: base, swapQuoteUnion: swapQuoteUnion)
    } else if let swapQuoteErrorUnion = swapQuoteErrorUnion {
      await self.handleSwapQuoteError(swapQuoteErrorUnion)
    } else {  // unknown error, ex failed parsing zerox quote.
      self.state = .error(Strings.Wallet.unknownError)
      self.clearAllAmount()
    }
  }

  @MainActor private func fetchSolPriceQuote(
    base: SwapParamsBase,
    swapQuoteParams: BraveWallet.SwapQuoteParams
  ) async {
    self.isUpdatingPriceQuote = true
    defer { self.isUpdatingPriceQuote = false }
    let (swapQuoteUnion, swapFees, swapQuoteErrorUnion, _) = await swapService.quote(
      params: swapQuoteParams
    )
    guard !Task.isCancelled else { return }
    self.currentSwapQuoteInfo = .init(
      base: base,
      swapQuote: swapQuoteUnion,
      swapFees: swapFees
    )
    if let swapQuoteUnion = swapQuoteUnion {
      await self.handleSwapQuote(base: base, swapQuoteUnion: swapQuoteUnion)
    } else if let swapQuoteErrorUnion = swapQuoteErrorUnion {
      await self.handleSwapQuoteError(swapQuoteErrorUnion)
    } else {  // unknown error, ex failed parsing jupiter quote.
      self.state = .error(Strings.Wallet.unknownError)
      self.clearAllAmount()
    }
  }

  @MainActor private func handleSwapQuote(
    base: SwapParamsBase,
    swapQuoteUnion: BraveWallet.SwapQuoteUnion
  ) async {
    if let zeroExQuote = swapQuoteUnion.zeroExQuote {
      await handleZeroExQuote(base: base, zeroExQuote: zeroExQuote)
    } else if let jupiterQuote = swapQuoteUnion.jupiterQuote {
      await handleJupiterQuote(jupiterQuote)
    } else if let lifiQuote = swapQuoteUnion.lifiQuote {
      await handleLifiQuote(base: base, lifiQuote)
    }
  }

  /// Update price market and sell/buy amount fields based on `SwapParamsBase` & `ZeroExQuote`
  @MainActor private func handleZeroExQuote(
    base: SwapParamsBase,
    zeroExQuote: BraveWallet.ZeroExQuote
  ) async {
    guard !Task.isCancelled else { return }
    let walletAmountFormatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))
    switch base {
    case .perSellAsset:
      var decimal = 18
      if let buyToken = selectedToToken {
        decimal = Int(buyToken.decimals)
      }
      let decimalString =
        walletAmountFormatter.decimalString(for: zeroExQuote.buyAmount, decimals: decimal) ?? ""
      if let bv = BDouble(decimalString) {
        buyAmount = bv.decimalDescription
      }
    case .perBuyAsset:
      var decimal = 18
      if let sellToken = selectedFromToken {
        decimal = Int(sellToken.decimals)
      }
      let decimalString =
        walletAmountFormatter.decimalString(for: zeroExQuote.sellAmount, decimals: decimal) ?? ""
      if let bv = BDouble(decimalString) {
        sellAmount = bv.decimalDescription
      }
    }

    if let bv = BDouble(zeroExQuote.price) {
      switch base {
      case .perSellAsset:
        selectedFromTokenPrice = bv.decimalDescription
      case .perBuyAsset:
        // will need to invert price if price quote is based on buyAmount
        if bv != 0 {
          selectedFromTokenPrice = (1 / bv).decimalDescription
        }
      }
    }

    guard let accountInfo,
      let gasLimit = BDouble(zeroExQuote.estimatedGas),
      let gasPrice = BDouble(zeroExQuote.gasPrice, over: "1000000000000000000"),
      let sellAmountValue = BDouble(sellAmount.normalizedDecimals),
      let fromToken = selectedFromToken
    else {
      self.state = .error(Strings.Wallet.unknownError)
      return
    }
    let network = await rpcService.network(coin: accountInfo.coin, origin: nil)

    // Check if balance available to pay for gas
    let ethBalance: BDouble
    if let assetBalance = assetManager.getAssetBalances(
      for: network.nativeToken,
      account: accountInfo.id
    )?.first(where: { $0.chainId == network.chainId }) {
      ethBalance = BDouble(assetBalance.balance) ?? 0
    } else {
      let (ethBalanceString, _, _) = await rpcService.balance(
        address: accountInfo.address,
        coin: network.coin,
        chainId: network.chainId
      )
      let balanceFormatter = WalletAmountFormatter(decimalFormatStyle: .balance)
      ethBalance =
        BDouble(
          balanceFormatter.decimalString(
            for: ethBalanceString.removingHexPrefix,
            radix: .hex,
            decimals: 18
          ) ?? ""
        ) ?? 0
    }
    let fee = gasLimit * gasPrice
    if fromToken.symbol == network.symbol {
      if ethBalance < fee + sellAmountValue {
        self.state = .error(Strings.Wallet.insufficientFundsForGas)
        return
      }
    } else {
      if ethBalance < fee {
        self.state = .error(Strings.Wallet.insufficientFundsForGas)
        return
      }
    }

    // Check allowance if erc20
    if fromToken.isErc20 {
      await self.checkAllowance(
        network: network,
        ownerAddress: accountInfo.address,
        spenderAddress: zeroExQuote.allowanceTarget,
        amountToSend: sellAmountValue,
        fromToken: fromToken
      )
    } else {
      self.state = .swap
    }
  }

  /// Update price market and sell/buy amount fields based on `JupiterQuote`
  @MainActor private func handleJupiterQuote(
    _ jupiterQuote: BraveWallet.JupiterQuote
  ) async {
    guard !Task.isCancelled else { return }
    let formatter = WalletAmountFormatter(decimalFormatStyle: .balance)
    if let selectedToToken {
      buyAmount =
        formatter.decimalString(
          for: jupiterQuote.outAmount,
          decimals: Int(selectedToToken.decimals)
        ) ?? ""
    }

    // No exchange rate is returned by Jupiter API, so we estimate it from the quote.
    if let selectedFromToken,
      let newFromAmount = formatter.decimalString(
        for: jupiterQuote.inAmount,
        decimals: Int(selectedFromToken.decimals)
      ),
      let newFromAmountWrapped = BDouble(newFromAmount),
      let selectedToToken,
      let newToAmount = formatter.decimalString(
        for: jupiterQuote.outAmount,
        decimals: Int(selectedToToken.decimals)
      ),
      let newToAmountWrapped = BDouble(newToAmount),
      newFromAmountWrapped != 0
    {
      let rate = newToAmountWrapped / newFromAmountWrapped
      selectedFromTokenPrice = rate.decimalDescription
    }

    // Validate balance available
    guard let sellAmountValue = BDouble(sellAmount.normalizedDecimals),
      let selectedFromTokenBalance
    else {
      self.state = .error(Strings.Wallet.unknownError)
      return
    }
    // Check if balance is insufficient
    guard sellAmountValue <= selectedFromTokenBalance else {
      state = .error(Strings.Wallet.insufficientBalance)
      return
    }
    // Cannot currently estimate SOL gas balance to verify
    self.state = .swap
  }

  @MainActor private func handleLifiQuote(
    base: SwapParamsBase,
    _ lifiQuote: BraveWallet.LiFiQuote
  ) async {
    guard !Task.isCancelled else { return }
    guard let route = lifiQuote.routes.first,
      let step = route.steps.first,
      // shouldn't occur, cross-chain token selection unavailable on iOS
      route.fromToken.coin == route.toToken.coin
    else {
      self.state = .error(Strings.Wallet.unknownError)
      return
    }
    let walletAmountFormatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))
    switch base {
    case .perSellAsset:
      let decimal = Int(route.toToken.decimals)
      let decimalString =
        walletAmountFormatter.decimalString(for: route.toAmount, decimals: decimal) ?? ""
      if let bv = BDouble(decimalString) {
        buyAmount = bv.decimalDescription
      }
    case .perBuyAsset:
      let decimal = Int(route.fromToken.decimals)
      let decimalString =
        walletAmountFormatter.decimalString(for: route.fromAmount, decimals: decimal) ?? ""
      if let bv = BDouble(decimalString) {
        sellAmount = bv.decimalDescription
      }
    }

    guard let accountInfo,
      let sellAmountValue = BDouble(sellAmount.normalizedDecimals)
    else {
      self.state = .error(Strings.Wallet.unknownError)
      return
    }
    let network = await rpcService.network(coin: accountInfo.coin, origin: nil)

    // Check if balance available to pay for gas
    if route.fromToken.coin == .eth {
      let ethBalance: BDouble
      if let assetBalance = assetManager.getAssetBalances(
        for: network.nativeToken,
        account: accountInfo.id
      )?.first(where: { $0.chainId == network.chainId }) {
        ethBalance = BDouble(assetBalance.balance) ?? 0
      } else {
        let (ethBalanceString, _, _) = await rpcService.balance(
          address: accountInfo.address,
          coin: network.coin,
          chainId: network.chainId
        )
        ethBalance =
          BDouble(
            walletAmountFormatter.decimalString(
              for: ethBalanceString.removingHexPrefix,
              radix: .hex,
              decimals: 18
            ) ?? ""
          ) ?? 0
      }
      let feeTotal: Double = step.estimate.gasCosts.reduce(Double(0)) { total, cost in
        total + (Double(cost.amount) ?? 0)
      }
      let fee =
        BDouble(
          walletAmountFormatter.decimalString(
            for: "\(feeTotal)",
            decimals: Int(network.decimals)
          ) ?? ""
        ) ?? 0
      if route.fromToken.symbol == network.symbol {
        if ethBalance < fee + sellAmountValue {
          self.state = .error(Strings.Wallet.insufficientFundsForGas)
          return
        }
      } else {
        if ethBalance < fee {
          self.state = .error(Strings.Wallet.insufficientFundsForGas)
          return
        }
      }
    }  // else fromToken.coin == .sol
    // same-chain SOL swaps not currently supported.
    // https://docs.li.fi/li.fi-api/solana

    // `isErc20` not assigned in `LiFiRoute` response
    let fromToken = selectedFromToken ?? route.fromToken
    // Check allowance if token is erc20
    if fromToken.isErc20 {
      await self.checkAllowance(
        network: network,
        ownerAddress: accountInfo.address,
        spenderAddress: step.estimate.approvalAddress,
        amountToSend: sellAmountValue,
        fromToken: fromToken
      )
    } else {
      self.state = .swap
    }
  }

  @MainActor private func handleSwapQuoteError(_ swapError: BraveWallet.SwapErrorUnion) async {
    // check balance first because error can cause by insufficient balance
    if let sellTokenBalance = self.selectedFromTokenBalance,
      let sellAmountValue = BDouble(self.sellAmount.normalizedDecimals),
      sellTokenBalance < sellAmountValue
    {
      self.state = .error(Strings.Wallet.insufficientBalance)
      return
    }

    // check if price quote fails due to insufficient liquidity
    if let zeroExError = swapError.zeroExError,
      zeroExError.isInsufficientLiquidity
    {
      self.state = .error(Strings.Wallet.insufficientLiquidity)
      return
    } else if let jupiterError = swapError.jupiterError,
      jupiterError.isInsufficientLiquidity
    {
      self.state = .error(Strings.Wallet.insufficientLiquidity)
      return
    } else if let lifiError = swapError.lifiError,
      lifiError.code == .notFoundError
    {
      self.state = .error(Strings.Wallet.insufficientLiquidity)
      return
    }
    self.state = .error(Strings.Wallet.unknownError)
    self.clearAllAmount()
  }

  @MainActor private func createSolSwapTransaction() async -> Bool {
    guard let jupiterQuote = currentSwapQuoteInfo?.swapQuote?.jupiterQuote,
      let accountInfo = self.accountInfo
    else {
      return false
    }
    self.isMakingTx = true
    defer { self.isMakingTx = false }
    let network = await rpcService.network(coin: .sol, origin: nil)

    let jupiterTransactionParams: BraveWallet.JupiterTransactionParams = .init(
      quote: jupiterQuote,
      chainId: network.chainId,
      userPublicKey: accountInfo.address
    )
    let (swapTransactionUnion, errorResponseUnion, _) = await swapService.transaction(
      params: .init(jupiterTransactionParams: jupiterTransactionParams)
    )
    guard let jupiterTransaction = swapTransactionUnion?.jupiterTransaction else {
      // check balance first because error can cause by insufficient balance
      if let sellTokenBalance = self.selectedFromTokenBalance,
        let sellAmountValue = BDouble(self.sellAmount.normalizedDecimals),
        sellTokenBalance < sellAmountValue
      {
        self.state = .error(Strings.Wallet.insufficientBalance)
        return false
      }
      // check if jupiterQuote fails due to insufficient liquidity
      if let errorResponse = errorResponseUnion?.jupiterError,
        errorResponse.isInsufficientLiquidity == true
      {
        self.state = .error(Strings.Wallet.insufficientLiquidity)
        return false
      }
      self.state = .error(Strings.Wallet.unknownError)
      return false
    }
    let (solTxData, status, _) = await solTxManagerProxy.makeTxDataFromBase64EncodedTransaction(
      jupiterTransaction,
      txType: .solanaSwap,
      sendOptions: .init(
        maxRetries: .init(maxRetries: 3),
        preflightCommitment: "processed",
        skipPreflight: .init(skipPreflight: true)
      )
    )
    guard status == .success, let solTxData else {
      self.state = .error(Strings.Wallet.unknownError)
      return false
    }
    let (success, _, _) = await txService.addUnapprovedTransaction(
      txDataUnion: .init(solanaTxData: solTxData),
      chainId: network.chainId,
      from: accountInfo.accountId
    )
    return success
  }

  // MARK: Public

  func swapSelectedTokens() {
    guard let oldFromToken = selectedFromToken,
      let oldToToken = selectedToToken
    else { return }

    selectedFromToken = oldToToken
    selectedToToken = oldFromToken

    if !sellAmount.isEmpty {
      fetchPriceQuote(base: .perSellAsset)
    }
  }

  @MainActor func createSwapTransaction() async -> Bool {
    guard let accountInfo else {
      return false
    }
    switch state {
    case .error, .idle:
      // will never come here
      return false
    case .lowAllowance(let spenderAddress):
      return await createERC20ApprovalTransaction(spenderAddress)
    case .swap:
      switch accountInfo.coin {
      case .eth:
        return await createEthSwapTransaction()
      case .sol:
        return await createSolSwapTransaction()
      default:
        return false
      }
    }
  }

  private var priceQuoteTask: Task<(), Never>?
  func fetchPriceQuote(base: SwapParamsBase) {
    priceQuoteTask?.cancel()
    priceQuoteTask = Task { @MainActor in
      // reset quote before fetching new quote
      self.currentSwapQuoteInfo = nil
      guard let accountInfo else {
        self.state = .idle
        return
      }
      let network = await rpcService.network(coin: accountInfo.coin, origin: nil)
      guard !Task.isCancelled else { return }
      // Entering a buy amount is disabled for Solana swaps, always use
      // `SwapParamsBase.perSellAsset` to fetch quote based on the sell amount.
      // `SwapParamsBase.perBuyAsset` is sent when `selectedToToken` is changed.
      guard
        let swapQuoteParams = self.swapQuoteParameters(
          for: accountInfo.coin == .sol ? .perSellAsset : base,
          in: network
        )
      else {
        self.state = .idle
        return
      }
      switch accountInfo.coin {
      case .eth:
        await fetchEthPriceQuote(base: base, swapQuoteParams: swapQuoteParams)
      case .sol:
        await fetchSolPriceQuote(base: base, swapQuoteParams: swapQuoteParams)
      default:
        break
      }
    }
  }

  func prepare(with accountInfo: BraveWallet.AccountInfo, completion: (() -> Void)? = nil) {
    self.accountInfo = accountInfo

    func updateSelectedTokens(in network: BraveWallet.NetworkInfo) {
      if let fromToken = selectedFromToken {  // refresh balance
        fetchTokenBalance(for: fromToken) { [weak self] balance in
          self?.selectedFromTokenBalance = balance ?? 0
        }
      } else {
        selectedFromToken = allTokens.first(where: { $0.symbol == network.symbol })
      }

      if let toToken = selectedToToken {
        fetchTokenBalance(for: toToken) { [weak self] balance in
          self?.selectedToTokenBalance = balance ?? 0
        }
      } else {
        if network.chainId == BraveWallet.MainnetChainId {
          if let fromToken = selectedFromToken,
            fromToken.symbol.uppercased() == batSymbol.uppercased()
          {
            selectedToToken = allTokens.first(where: {
              $0.symbol.uppercased() != batSymbol.uppercased()
            })
          } else if let batToken = allTokens.first(where: {
            $0.symbol.uppercased() == batSymbol.uppercased()
          }) {
            selectedToToken = batToken
          } else {  // if BAT is unavailable
            selectedToToken = allTokens.first(where: {
              $0.symbol.uppercased() != selectedFromToken?.symbol.uppercased()
            })
          }
        } else if network.chainId == BraveWallet.SolanaMainnet {
          if let fromToken = selectedFromToken,
            fromToken.symbol.uppercased() == usdcSymbol.uppercased()
          {
            selectedToToken = allTokens.first(where: {
              $0.symbol.uppercased() != usdcSymbol.uppercased()
            })
          } else if let usdcToken = allTokens.first(where: {
            $0.symbol.uppercased() == usdcSymbol.uppercased()
          }) {
            selectedToToken = usdcToken
          } else {  // if USDC is unavailable
            selectedToToken = allTokens.first(where: {
              $0.symbol.uppercased() != selectedFromToken?.symbol.uppercased()
            })
          }
        } else {
          if let fromToken = selectedFromToken {
            selectedToToken = allTokens.first(where: {
              $0.symbol.uppercased() != fromToken.symbol.uppercased()
            })
          } else {
            selectedToToken = allTokens.first
          }
        }
        completion?()
      }
    }

    // All tokens from token registry
    self.rpcService.network(coin: accountInfo.coin, origin: nil) { network in
      // Closure run after validating the prefilledToken (if applicable)
      let continueClosure: (BraveWallet.NetworkInfo) -> Void = { [weak self] network in
        guard let self = self else { return }
        self.blockchainRegistry.allTokens(chainId: network.chainId, coin: network.coin) { tokens in
          Task { @MainActor in
            let nativeAsset = network.nativeToken
            // visible custom tokens added by users
            let userVisibleAssets = await self.assetManager.getUserAssets(
              networks: [network],
              visible: true
            ).flatMap { $0.tokens }
            let customTokens = userVisibleAssets.filter { asset in
              !tokens.contains(where: {
                $0.contractAddress(in: network).caseInsensitiveCompare(asset.contractAddress)
                  == .orderedSame
              })
            }
            let sortedCustomTokens = customTokens.sorted {
              if $0.contractAddress(in: network).caseInsensitiveCompare(nativeAsset.contractAddress)
                == .orderedSame
              {
                return true
              } else {
                return $0.symbol < $1.symbol
              }
            }
            self.allTokens = (sortedCustomTokens + tokens.sorted(by: { $0.symbol < $1.symbol }))
              .filter { !$0.isNft }
            // Seems like user assets always include the selected network's native asset
            // But let's make sure all token list includes the native asset
            if !self.allTokens.contains(where: {
              $0.symbol.lowercased() == nativeAsset.symbol.lowercased()
            }) {
              self.allTokens.insert(nativeAsset, at: 0)
            }
            updateSelectedTokens(in: network)
          }
          // Native token on the current selected network
        }
      }

      // validate the `prefilledToken`
      if let prefilledToken = self.prefilledToken {
        if prefilledToken.coin == network.coin && prefilledToken.chainId == network.chainId {
          self.selectedFromToken = prefilledToken
          continueClosure(network)
        } else {
          self.rpcService.allNetworks { allNetworks in
            guard
              let networkForToken = allNetworks.first(where: {
                $0.coin == prefilledToken.coin && $0.chainId == prefilledToken.chainId
              })
            else {
              // don't set prefilled token if it belongs to a network we don't know
              continueClosure(network)
              return
            }
            self.rpcService.setNetwork(
              chainId: networkForToken.chainId,
              coin: networkForToken.coin,
              origin: nil
            ) { success in
              if success {
                self.selectedFromToken = prefilledToken
                continueClosure(networkForToken)  // network changed
              } else {
                continueClosure(network)
              }
            }
          }
        }
        self.prefilledToken = nil
      } else {  // `prefilledToken` is nil
        continueClosure(network)
      }
    }
  }

  func suggestedAmountTapped(_ amount: ShortcutAmountGrid.Amount) {
    var decimalPoint = 6
    var rounded = true
    if amount == .all {
      decimalPoint = Int(selectedFromToken?.decimals ?? 18)
      rounded = false
    }
    sellAmount =
      ((selectedFromTokenBalance ?? 0) * amount.rawValue)
      .decimalExpansion(precisionAfterDecimalPoint: decimalPoint, rounded: rounded)
      .trimmingTrailingZeros
  }

  #if DEBUG
  func setUpTest(
    fromAccount: BraveWallet.AccountInfo = .mockEthAccount,
    selectedFromToken: BraveWallet.BlockchainToken = .previewToken,
    selectedToToken: BraveWallet.BlockchainToken = .previewDaiToken,
    sellAmount: String? = "0.01",
    buyAmount: String? = nil,
    currentSwapQuoteInfo: CurrentSwapQuoteInfo? = nil
  ) {
    accountInfo = fromAccount
    self.selectedFromToken = selectedFromToken
    self.selectedToToken = selectedToToken
    if let sellAmount {
      self.sellAmount = sellAmount
    }
    if let buyAmount {
      self.buyAmount = buyAmount
    }
    if let currentSwapQuoteInfo {
      self.currentSwapQuoteInfo = currentSwapQuoteInfo
    }
    selectedFromTokenBalance = 0.02
  }

  func setUpTestForRounding() {
    accountInfo = .init()
    selectedFromToken = .previewToken
  }
  #endif
}

extension SwapTokenStore: WalletUserAssetDataObserver {
  public func cachedBalanceRefreshed() {
    if let token = selectedFromToken {
      fetchTokenBalance(for: token) { [weak self] balance in
        self?.selectedFromTokenBalance = balance
      }
    }
  }

  public func userAssetUpdated() {
  }
}
