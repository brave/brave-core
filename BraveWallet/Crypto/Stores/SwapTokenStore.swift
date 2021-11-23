// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BigNumber
import Shared

/// A store contains data for swap tokens
public class SwapTokenStore: ObservableObject {
  /// All  tokens for searching use
  @Published var allTokens: [BraveWallet.ERCToken] = []
  /// The current selected token to swap from. Default with nil value.
  @Published var selectedFromToken: BraveWallet.ERCToken? {
    didSet {
      if let token = selectedFromToken {
        fetchTokenBalance(for: token) { [weak self] balance in
          self?.selectedFromTokenBalance = balance
        }
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
  @Published var selectedFromTokenBalance: BDouble?
  /// The current selected token balance to swap to. Default with nil value.
  @Published var selectedToTokenBalance: BDouble?
  /// The current market price for selected token to swap from.
  @Published var selectedFromTokenPrice = "0"
  /// The state of swap screen
  @Published var state: SwapState = .idle
  /// The sell amount in this swap
  @Published var sellAmount = "" {
    didSet {
      guard !sellAmount.isEmpty, BDouble(sellAmount) != nil else {
        state = .idle
        return
      }
      if oldValue != sellAmount && !updatingPriceQuote {
        timer?.invalidate()
        timer = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: false, block: { [weak self] _ in
          self?.fetchPriceQuote(base: .perSellAsset)
        })
      }
    }
  }
  /// The buy amount in this swap
  @Published var buyAmount = "" {
    didSet {
      guard !buyAmount.isEmpty, BDouble(buyAmount) != nil else {
        state = .idle
        return
      }
      if oldValue != buyAmount && !updatingPriceQuote {
        timer?.invalidate()
        timer = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: false, block: { [weak self] _ in
          self?.fetchPriceQuote(base: .perBuyAsset)
        })
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
  
  private let keyringController: BraveWalletKeyringController
  private let tokenRegistry: BraveWalletERCTokenRegistry
  private let rpcController: BraveWalletEthJsonRpcController
  private let assetRatioController: BraveWalletAssetRatioController
  private let swapController: BraveWalletSwapController
  private let transactionController: BraveWalletEthTxController
  private var accountInfo: BraveWallet.AccountInfo?
  private var chainId: String = ""
  private var slippage = 0.005 {
    didSet {
      timer?.invalidate()
      timer = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: false, block: { [weak self] _ in
        self?.fetchPriceQuote(base: .perSellAsset)
      })
    }
  }
  private var updatingPriceQuote = false
  private var timer: Timer?
  
  enum SwapParamsBase {
    // calculating based on sell asset amount
    case perSellAsset
    // calculating based on buy asset amount
    case perBuyAsset
  }
  
  enum SwapState {
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
    keyringController: BraveWalletKeyringController,
    tokenRegistry: BraveWalletERCTokenRegistry,
    rpcController: BraveWalletEthJsonRpcController,
    assetRatioController: BraveWalletAssetRatioController,
    swapController: BraveWalletSwapController,
    transactionController: BraveWalletEthTxController
  ) {
    self.keyringController = keyringController
    self.tokenRegistry = tokenRegistry
    self.rpcController = rpcController
    self.assetRatioController = assetRatioController
    self.swapController = swapController
    self.transactionController = transactionController
    
    self.keyringController.add(self)
    self.rpcController.add(self)
  }
  
  private func fetchTokenBalance(
    for token: BraveWallet.ERCToken,
    completion: @escaping (_ balance: BDouble?) -> Void
  ) {
    guard let account = accountInfo else {
      completion(nil)
      return
    }
    
    rpcController.balance(for: token, in: account) { balance in
      completion(BDouble(balance ?? 0))
    }
  }
  
  private func swapParameters(for base: SwapParamsBase) -> BraveWallet.SwapParams? {
    guard
      let accountInfo = accountInfo,
      let sellToken = selectedFromToken,
      let buyToken = selectedToToken
    else { return nil }
    
    let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    let sellAddress = sellToken.swapAddress(in: chainId)
    let buyAddress = buyToken.swapAddress(in: chainId)
    let sellAmountInWei: String
    let buyAmountInWei: String
    switch base {
    case .perSellAsset:
      // make sure the base value should not be zero, otherwise, it will always return insufficient liquidity error.
      // following desktop to make a idle state
      if let sellAmountValue = BDouble(sellAmount), sellAmountValue == 0 {
        return nil
      }
      sellAmountInWei = weiFormatter.weiString(from: sellAmount, radix: .decimal, decimals: Int(sellToken.decimals)) ?? "0"
      buyAmountInWei = ""
    case .perBuyAsset:
      // same as sell amount. make sure base value should not be zero
      if let buyAmountValue = BDouble(buyAmount), buyAmountValue == 0 {
        return nil
      }
      sellAmountInWei = ""
      buyAmountInWei = weiFormatter.weiString(from: buyAmount, radix: .decimal, decimals: Int(buyToken.decimals)) ?? "0"
    }
    let swapParams = BraveWallet.SwapParams(
      takerAddress: accountInfo.address,
      sellAmount: sellAmountInWei,
      buyAmount: buyAmountInWei,
      buyToken: buyAddress,
      sellToken: sellAddress,
      slippagePercentage: slippage,
      gasPrice: ""
    )
    
    return swapParams
  }
  
  private func createETHSwapTransaction() {
    guard
      let accountInfo = accountInfo,
      let swapParams = swapParameters(for: .perSellAsset)
    else { return }
    
    rpcController.network { [weak self] network in
      guard let self = self else {
        self?.state = .error(Strings.Wallet.unknownError)
        self?.clearAllAmount()
        return
      }
      self.swapController.transactionPayload(swapParams) { success, swapResponse, error in
        guard success else {
          self.state = .error(Strings.Wallet.unknownError)
          self.clearAllAmount()
          return
        }
        guard let response = swapResponse else {
          self.state = .error(Strings.Wallet.unknownError)
          self.clearAllAmount()
          return
        }
        
        let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
        let gasPrice = "0x\(weiFormatter.weiString(from: response.gasPrice, radix: .hex, decimals: 0) ?? "0")" // already in wei
        let gasLimit = "0x\(weiFormatter.weiString(from: response.estimatedGas, radix: .hex, decimals: 0) ?? "0")" // already in wei
        let value = "0x\(weiFormatter.weiString(from: response.value, radix: .hex, decimals: 0) ?? "0")" // already in wei
        let data: [NSNumber] = .init(hexString: response.data) ?? .init()
        
        if network.isEip1559 {
          let baseData: BraveWallet.TxData = .init(
            nonce: "",
            gasPrice: "", // no gas price in eip1559
            gasLimit: gasLimit,
            to: response.to,
            value: value,
            data: data
          )
          self.makeEIP1559Tx(chainId: network.chainId,
                             baseData: baseData,
                             from: accountInfo) { success in
            // should be observed
            guard success else {
              self.state = .error(Strings.Wallet.unknownError)
              self.clearAllAmount()
              return
            }
          }
        } else {
          let baseData: BraveWallet.TxData = .init(
            nonce: "",
            gasPrice: gasPrice,
            gasLimit: gasLimit,
            to: response.to,
            value: value,
            data: data
          )
          self.transactionController.addUnapprovedTransaction(baseData, from: accountInfo.address) { success, txMetaId, error in
            // should be observed
            guard success else {
              self.state = .error(Strings.Wallet.unknownError)
              self.clearAllAmount()
              return
            }
          }
        }
      }
    }
  }
  
  private func isLiquidityError(_ error: String) -> Bool {
    guard
      let data = error.data(using: .utf8, allowLossyConversion: false),
      let object = try? JSONSerialization.jsonObject(with: data, options: .mutableContainers) as? [String: Any],
      let validationErrors = object["validationErrors"] as? [[String: Any]]
    else { return false }
    
    for error in validationErrors {
      if let reason = error["reason"] as? String, reason == "INSUFFICIENT_ASSET_LIQUIDITY" {
        return true
      }
    }
    
    return false
  }
  
  private func clearAllAmount() {
    sellAmount = "0"
    buyAmount = "0"
    selectedFromTokenPrice = "0"
  }
  
  /// Update price market and sell/buy amount fields based on `SwapParamsBase`
  private func handlePriceQuoteResponse(_ response: BraveWallet.SwapResponse, base: SwapParamsBase) {
    let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    switch base {
    case .perSellAsset:
      var decimal = 18
      if let buyToken = selectedToToken {
        decimal = Int(buyToken.decimals)
      }
      let decimalString = weiFormatter.decimalString(for: response.buyAmount, decimals: decimal) ?? ""
      if let bv = BDouble(decimalString) {
        buyAmount = bv.decimalDescription
      }
    case .perBuyAsset:
      var decimal = 18
      if let sellToken = selectedFromToken {
        decimal = Int(sellToken.decimals)
      }
      let decimalString = weiFormatter.decimalString(for: response.sellAmount, decimals: decimal) ?? ""
      if let bv = BDouble(decimalString) {
        sellAmount = bv.decimalDescription
      }
    }
    
    if let bv = BDouble(response.price) {
      // will need to invert price if price quote is based on buyAmount
      // ref from slack:
      // https://bravesoftware.slack.com/archives/C023VS4HJ6Q/p1636579425364500?thread_ts=1636570735.354500&cid=C023VS4HJ6Q
      let price = base == .perSellAsset ? bv : 1 / bv
      selectedFromTokenPrice = price.decimalDescription
    }
    
    checkBalanceShowError(swapResponse: response)
  }
  
  private func createERC20SwapTransaction(_ spenderAddress: String) {
    let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    guard
      let fromToken = selectedFromToken,
      let accountInfo = accountInfo,
      let balanceInWeiHex = weiFormatter.weiString(
        from: selectedFromTokenBalance?.decimalDescription ?? "",
        radix: .hex,
        decimals: Int(fromToken.decimals)
      )
    else { return }
    
    rpcController.network { [weak self] network in
      guard let self = self else { return }
      self.transactionController.makeErc20ApproveData(
        spenderAddress,
        amount: balanceInWeiHex
      ) { success, data in
        guard success else { return }
        let baseData = BraveWallet.TxData(
          nonce: "",
          gasPrice: "",
          gasLimit: "",
          to: fromToken.contractAddress,
          value: "0x0",
          data: data
        )
        if network.isEip1559 {
          self.makeEIP1559Tx(chainId: network.chainId,
                             baseData: baseData,
                             from: accountInfo) { success in
            guard success else {
              self.state = .error(Strings.Wallet.unknownError)
              self.clearAllAmount()
              return
            }
          }
        } else {
          self.transactionController.addUnapprovedTransaction(
              baseData,
              from: accountInfo.address,
              completion: { success, txMetaId, error in
                // should be observed
                guard success else {
                  self.state = .error(Strings.Wallet.unknownError)
                  self.clearAllAmount()
                  return
                }
              }
          )
        }
      }
    }
  }
  
  private func makeEIP1559Tx(
    chainId: String,
    baseData: BraveWallet.TxData,
    from account: BraveWallet.AccountInfo,
    completion: @escaping (_ success: Bool) -> Void
  ) {
    var maxPriorityFeePerGas = ""
    var maxFeePerGas = ""
    assetRatioController.gasOracle { [weak self] gasEstimation in
      guard let self = self else { return }
      if let gasEstimation = gasEstimation {
        // Bump fast priority fee and max fee by 1 GWei if same as average fees.
        if gasEstimation.fastMaxPriorityFeePerGas == gasEstimation.avgMaxPriorityFeePerGas {
          maxPriorityFeePerGas = "0x\(self.bumpFeeByOneGWei(with: gasEstimation.fastMaxPriorityFeePerGas) ?? "0")"
          maxFeePerGas = "0x\(self.bumpFeeByOneGWei(with: gasEstimation.fastMaxFeePerGas) ?? "0")"
        } else {
          // Always suggest fast gas fees as default
          maxPriorityFeePerGas = gasEstimation.fastMaxPriorityFeePerGas
          maxFeePerGas = gasEstimation.fastMaxFeePerGas
        }
      }
      let eip1559Data = BraveWallet.TxData1559(baseData: baseData, chainId: chainId, maxPriorityFeePerGas: maxPriorityFeePerGas, maxFeePerGas: maxFeePerGas, gasEstimation: gasEstimation)
      self.transactionController.addUnapproved1559Transaction(eip1559Data, from: account.address) { success, txMetaId, errorMessage in
        completion(success)
      }
    }
  }
  
  private func bumpFeeByOneGWei(with value: String) -> String? {
    guard let bv = BDouble(value) else { return nil }
    let bumpedValue = bv + (BDouble(10) ** 9)
    return bumpedValue.rounded().asString(radix: 16)
  }
  
  private func checkBalanceShowError(swapResponse: BraveWallet.SwapResponse) {
    guard
      let accountInfo = accountInfo,
      let sellAmountValue = BDouble(sellAmount),
      let gasLimit = BDouble(swapResponse.estimatedGas),
      let gasPrice = BDouble(swapResponse.gasPrice, over: "1000000000000000000"),
      let fromToken = selectedFromToken,
      let fromTokenBalance = selectedFromTokenBalance
    else { return }
    
    // Check if balance is insufficient
    guard sellAmountValue <= fromTokenBalance else {
      state = .error(Strings.Wallet.insufficientBalance)
      return
    }
    
    // Get ETH balance for this account because gas can only be paid in ETH
    rpcController.balance(accountInfo.address) { [weak self] success, balance in
      guard let self = self else { return }
      if success {
        let fee = gasLimit * gasPrice
        let balanceFormatter = WeiFormatter(decimalFormatStyle: .balance)
        let currentBalance = BDouble(balanceFormatter.decimalString(for: balance.removingHexPrefix, radix: .hex, decimals: 18) ?? "") ?? 0
        if fromToken.isETH {
          if currentBalance < fee + sellAmountValue {
            self.state = .error(Strings.Wallet.insufficientFundsForGas)
            return
          }
        } else {
          if currentBalance < fee {
            self.state = .error(Strings.Wallet.insufficientFundsForGas)
            return
          }
        }
        self.state = .swap
        // check for ERC20 token allowance
        if fromToken.isErc20 {
          self.checkAllowance(
            contractAddress: accountInfo.address,
            spenderAddress: swapResponse.allowanceTarget,
            amountToSend: sellAmountValue,
            fromToken: fromToken
          )
        }
      } else {
        self.state = .error(Strings.Wallet.unknownError)
        self.clearAllAmount()
      }
    }
  }
  
  private func checkAllowance(
    contractAddress: String,
    spenderAddress: String,
    amountToSend: BDouble,
    fromToken: BraveWallet.ERCToken
  ) {
    rpcController.erc20TokenAllowance(
      contractAddress,
      ownerAddress: contractAddress,
      spenderAddress: spenderAddress
    ) { [weak self] success, allowance in
      let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
      let allowanceValue = BDouble(weiFormatter.decimalString(for: allowance.removingHexPrefix, radix: .hex, decimals: Int(fromToken.decimals)) ?? "") ?? 0
      guard success, amountToSend > allowanceValue else { return } // no problem with its allowance
      self?.state = .lowAllowance(spenderAddress)
    }
  }
  
  // MARK: Public
  
  func prepareSwap() {
    switch state {
    case .error(_), .idle:
      // will never come here
      break
    case .lowAllowance(let spenderAddress):
      createERC20SwapTransaction(spenderAddress)
    case .swap:
      createETHSwapTransaction()
    }
  }
  
  func fetchPriceQuote(base: SwapParamsBase) {
    guard let swapParams = swapParameters(for: base) else {
      state = .idle
      return
    }
    
    updatingPriceQuote = true
    swapController.priceQuote(swapParams) { [weak self] success, response, error in
      guard let self = self else { return }
      defer { self.updatingPriceQuote = false}
      if success, let response = response {
        self.handlePriceQuoteResponse(response, base: base)
      } else {
        self.clearAllAmount()
        
        // check balance first because error can cause by insufficient balance
        if let sellTokenBalance = self.selectedFromTokenBalance,
           let sellAmountValue = BDouble(self.sellAmount),
           sellTokenBalance < sellAmountValue {
          self.state = .error(Strings.Wallet.insufficientBalance)
          return
        }
        // check if priceQuote fails due to insufficient liquidity
        if let error = error, self.isLiquidityError(error) {
          self.state = .error(Strings.Wallet.insufficientLiquidity)
          return
        }
        self.state = .error(Strings.Wallet.unknownError)
      }
    }
  }
  
  func prepare(with accountInfo: BraveWallet.AccountInfo, completion: (() -> Void)? = nil) {
    self.accountInfo = accountInfo
    
    func updateSelectedTokens(chainId: String) {
      if let fromToken = selectedFromToken { // refresh balance
        rpcController.balance(for: fromToken, in: accountInfo) { [weak self] balance in
          self?.selectedFromTokenBalance = BDouble(balance ?? 0)
        }
      } else {
        selectedFromToken = allTokens.first(where: { $0.isETH })
      }
      
      if let toToken = selectedToToken {
        rpcController.balance(for: toToken, in: accountInfo) { [weak self] balance in
          self?.selectedToTokenBalance = BDouble(balance ?? 0)
          completion?()
        }
      } else {
        if chainId == BraveWallet.MainnetChainId {
          selectedToToken = allTokens.first(where: { $0.symbol.uppercased() == "BAT" })
        } else if chainId == BraveWallet.RopstenChainId {
          selectedToToken = allTokens.first(where: { $0.symbol.uppercased() == "DAI" })
        }
        completion?()
      }
    }
    
    tokenRegistry.allTokens { [weak self] tokens in
      guard let self = self else { return }
      
      self.rpcController.chainId { chainId in
        self.chainId = chainId
        if chainId == BraveWallet.RopstenChainId {
          let supportedAssets = tokens.filter { BraveWallet.assetsSwapInRopsten.contains($0.symbol) } + [.eth]
          self.allTokens = supportedAssets.sorted(by: { $0.symbol < $1.symbol })
          updateSelectedTokens(chainId: chainId)
        } else {
          self.tokenRegistry.allTokens { [self] tokens in
            let fullList = tokens + [.eth]
            self.allTokens = fullList.sorted(by: { $0.symbol < $1.symbol })
            updateSelectedTokens(chainId: chainId)
          }
        }
      }
    }
  }
}

extension SwapTokenStore: BraveWalletKeyringControllerObserver {
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
    keyringController.defaultKeyringInfo { [self] keyringInfo in
      if !keyringInfo.accountInfos.isEmpty {
        keyringController.selectedAccount { accountAddress in
          let selectedAccountInfo = keyringInfo.accountInfos.first(where: { $0.address == accountAddress }) ??
            keyringInfo.accountInfos.first!
          prepare(with: selectedAccountInfo) {
            fetchPriceQuote(base: .perSellAsset)
          }
        }
      }
    }
  }
}

extension SwapTokenStore: BraveWalletEthJsonRpcControllerObserver {
  public func chainChangedEvent(_ chainId: String) {
    guard
      let accountInfo = accountInfo,
      chainId == BraveWallet.MainnetChainId || chainId == BraveWallet.RopstenChainId
    else { return }
    selectedFromToken = nil
    selectedToToken = nil
    prepare(with: accountInfo) { [weak self] in
      self?.fetchPriceQuote(base: .perSellAsset)
    }
  }
  
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
