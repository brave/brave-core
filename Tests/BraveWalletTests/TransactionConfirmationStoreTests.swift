// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
@testable import BraveWallet

@MainActor class TransactionConfirmationStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  private func setupStore(
    selectedNetworkForCoinType: [BraveWallet.CoinType: BraveWallet.NetworkInfo] = [
      .eth : BraveWallet.NetworkInfo.mockMainnet,
      .sol : BraveWallet.NetworkInfo.mockSolana
    ],
    accountInfos: [BraveWallet.AccountInfo] = [.mockEthAccount, .mockSolAccount],
    allTokens: [BraveWallet.BlockchainToken] = [],
    transactions: [BraveWallet.TransactionInfo] = [],
    gasEstimation: BraveWallet.GasEstimation1559 = .init(),
    makeErc20ApproveDataSuccess: Bool = true,
    setDataForUnapprovedTransactionSuccess: Bool = true
  ) -> TransactionConfirmationStore {
    let mockEthAssetPrice: BraveWallet.AssetPrice = .init(fromAsset: "eth", toAsset: "usd", price: "3059.99", assetTimeframeChange: "-57.23")
    let mockSolAssetPrice: BraveWallet.AssetPrice = .init(fromAsset: "sol", toAsset: "usd", price: "39.57", assetTimeframeChange: "-57.23")
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    let mockBalanceWei = formatter.weiString(from: 0.0896, radix: .hex, decimals: 18) ?? ""
    // setup test services
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { _, _, _, completion in
      completion(true, [mockEthAssetPrice, mockSolAssetPrice])
    }
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._chainIdForOrigin = { coin, origin, completion in
      completion(selectedNetworkForCoinType[coin]?.chainId ?? BraveWallet.MainnetChainId)
    }
    rpcService._network = { coin, origin, completion in
      completion(selectedNetworkForCoinType[coin] ?? .mockMainnet)
    }
    rpcService._balance = { _, _, _, completion in
      completion(mockBalanceWei, .success, "")
    }
    rpcService._erc20TokenAllowance = { _, _, _, _, completion in
      completion("16345785d8a0000", .success, "") // 0.1000
    }
    let txService = BraveWallet.TestTxService()
    txService._addObserver = { _ in }
    txService._allTransactionInfo = { _, _, _, completion in
      completion(transactions)
    }
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._allTokens = { _, _, completion in
      completion(allTokens)
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { _, _, completion in
      completion([])
    }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    walletService._addObserver = { _ in }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.eth) }
    let ethTxManagerProxy = BraveWallet.TestEthTxManagerProxy()
    ethTxManagerProxy._gasEstimation1559 = { _, completion in
      completion(gasEstimation)
    }
    ethTxManagerProxy._makeErc20ApproveData = { _, _, completion in
      completion(makeErc20ApproveDataSuccess, [])
    }
    ethTxManagerProxy._setDataForUnapprovedTransaction = { _, _, _, completion in
      completion(setDataForUnapprovedTransactionSuccess)
    }
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._keyringInfo = { id, completion in
      let keyring: BraveWallet.KeyringInfo = .init(
        id: id,
        isKeyringCreated: true,
        isLocked: false,
        isBackedUp: true,
        accountInfos: accountInfos)
      completion(keyring)
    }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._estimatedTxFee = { $2(0, .success, "") }
    
    return TransactionConfirmationStore(
      assetRatioService: assetRatioService,
      rpcService: rpcService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      keyringService: keyringService,
      solTxManagerProxy: solTxManagerProxy
    )
  }
  
  /// Test `prepare()`  update `state` data for symbol, value, isUnlimitedApprovalRequested.
  func testPrepareSolSystemTransfer() async {
    let mockAllTokens: [BraveWallet.BlockchainToken] = [.mockSolToken, .mockSpdToken]
    let mockTransaction: BraveWallet.TransactionInfo = .previewConfirmedSolSystemTransfer
    let mockTransactions: [BraveWallet.TransactionInfo] = [mockTransaction].map { tx in
      tx.txStatus = .unapproved
      return tx
    }
    let mockGasEstimation: BraveWallet.GasEstimation1559 = .init()
    let store = setupStore(
      accountInfos: [.mockSolAccount],
      allTokens: mockAllTokens,
      transactions: mockTransactions,
      gasEstimation: mockGasEstimation
    )
    let prepareExpectation = expectation(description: "prepare")
    await store.prepare()
    store.$activeTransactionId
      .sink { id in
        defer { prepareExpectation.fulfill() }
        XCTAssertEqual(id, mockTransaction.id)
      }
      .store(in: &cancellables)
    
    store.$gasValue
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, "0")
      }
      .store(in: &cancellables)
    
    store.$gasSymbol
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, BraveWallet.BlockchainToken.mockSolToken.symbol)
      }
      .store(in: &cancellables)
    store.$symbol
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, BraveWallet.BlockchainToken.mockSolToken.symbol)
      }
      .store(in: &cancellables)
    store.$value
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, "0.1")
      }
      .store(in: &cancellables)
    store.$isUnlimitedApprovalRequested
      .dropFirst()
      .sink { value in
        XCTAssertFalse(value) // .mockSpdToken has 6 decimals
      }
      .store(in: &cancellables)

    await fulfillment(of: [prepareExpectation], timeout: 1)
  }
  
  /// Test `prepare()`  update `state` data for symbol, value, isUnlimitedApprovalRequested.
  func testPrepareSolTokenTransfer() async {
    let mockAllTokens: [BraveWallet.BlockchainToken] = [.mockSolToken, .mockSpdToken]
    let mockTransaction: BraveWallet.TransactionInfo = .previewConfirmedSolTokenTransfer
    let mockTransactions: [BraveWallet.TransactionInfo] = [mockTransaction].map { tx in
      tx.txStatus = .unapproved
      return tx
    }
    let mockGasEstimation: BraveWallet.GasEstimation1559 = .init()
    let store = setupStore(
      accountInfos: [.mockSolAccount],
      allTokens: mockAllTokens,
      transactions: mockTransactions,
      gasEstimation: mockGasEstimation
    )
    let prepareExpectation = expectation(description: "prepare")
    await store.prepare()
    store.$activeTransactionId
      .sink { id in
        defer { prepareExpectation.fulfill() }
        XCTAssertEqual(id, mockTransaction.id)
      }
      .store(in: &cancellables)
    
    store.$gasValue
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, "0")
      }
      .store(in: &cancellables)
    
    store.$gasSymbol
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, BraveWallet.BlockchainToken.mockSolToken.symbol)
      }
      .store(in: &cancellables)
    store.$symbol
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, BraveWallet.BlockchainToken.mockSpdToken.symbol)
      }
      .store(in: &cancellables)
    store.$value
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, "100") // .mockSpdToken has 6 decimals
      }
      .store(in: &cancellables)
    store.$isUnlimitedApprovalRequested
      .dropFirst()
      .sink { value in
        XCTAssertFalse(value) // .mockSpdToken has 6 decimals
      }
      .store(in: &cancellables)
    
    await fulfillment(of: [prepareExpectation], timeout: 1)
  }
  
  /// Test `prepare()`  update `state` data for symbol, value, isUnlimitedApprovalRequested.
  func testPrepareERC20Approve() async {
    let mockAllTokens: [BraveWallet.BlockchainToken] = [.previewToken, .daiToken]
    let mockTransaction: BraveWallet.TransactionInfo = .previewConfirmedERC20Approve
    let mockTransactions: [BraveWallet.TransactionInfo] = [mockTransaction].map { tx in
      tx.txStatus = .unapproved
      return tx
    }
    let mockGasEstimation: BraveWallet.GasEstimation1559 = .init()
    let store = setupStore(
      accountInfos: [.mockEthAccount],
      allTokens: mockAllTokens,
      transactions: mockTransactions,
      gasEstimation: mockGasEstimation
    )
    let prepareExpectation = expectation(description: "prepare")
    await store.prepare()
    store.$activeTransactionId
      .sink { id in
        defer { prepareExpectation.fulfill() }
        XCTAssertEqual(id, mockTransaction.id)
      }
      .store(in: &cancellables)
    store.$gasSymbol
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, BraveWallet.BlockchainToken.previewToken.symbol)
      }
      .store(in: &cancellables)
    store.$symbol
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, BraveWallet.BlockchainToken.daiToken.symbol)
      }
      .store(in: &cancellables)
    store.$value
      .dropFirst()
      .sink { value in
        XCTAssertEqual(value, "Unlimited")
      }
      .store(in: &cancellables)
    store.$isUnlimitedApprovalRequested
      .dropFirst()
      .collect(2)
      .sink { values in
        XCTAssertNotNil(values.last)
        XCTAssertTrue(values.last!)
      }
      .store(in: &cancellables)
    store.$currentAllowance
      .dropFirst()
      .collect(5)
      .sink { values in
        XCTAssertNotNil(values.last)
        XCTAssertEqual(values.last!, "0.1000")
      }
      .store(in: &cancellables)
    
    await fulfillment(of: [prepareExpectation], timeout: 1)
  }
  
  /// Test `editAllowance(txMetaId:spenderAddress:amount:completion)` will return false if we fail to make ERC20 approve data with `BraveWalletEthTxManagerProxy`
  func testEditAllowanceFailMakeERC20ApproveData() async {
    let mockAllTokens: [BraveWallet.BlockchainToken] = [.previewToken, .daiToken]
    let mockTransaction: BraveWallet.TransactionInfo = .previewConfirmedERC20Approve
    let mockTransactions: [BraveWallet.TransactionInfo] = [mockTransaction].map { tx in
      tx.txStatus = .unapproved
      return tx
    }
    let mockGasEstimation: BraveWallet.GasEstimation1559 = .init()
    let store = setupStore(
      accountInfos: [.mockEthAccount],
      allTokens: mockAllTokens,
      transactions: mockTransactions,
      gasEstimation: mockGasEstimation,
      makeErc20ApproveDataSuccess: false
    )
    
    let prepareExpectation = expectation(description: "prepare")
    await store.prepare()
    store.$activeTransactionId
      .sink { id in
        defer { prepareExpectation.fulfill() }
        XCTAssertEqual(id, mockTransaction.id)
      }
      .store(in: &cancellables)
    await fulfillment(of: [prepareExpectation], timeout: 1)
    
    let editExpectation = expectation(description: "edit allowance")
    store.editAllowance(
      transaction: mockTransaction,
      spenderAddress: mockTransaction.txArgs[safe: 0] ?? "",
      amount: "0x0",
      completion: { success in
        if !success {
          editExpectation.fulfill()
        }
      }
    )
    await fulfillment(of: [editExpectation], timeout: 1)
  }
  
  /// Test `editAllowance(txMetaId:spenderAddress:amount:completion)` will return false if we fail to set new ERC20 Approve data with `BraveWalletEthTxManagerProxy`
  func testEditAllowanceFailSetData() async {
    let mockAllTokens: [BraveWallet.BlockchainToken] = [.previewToken, .daiToken]
    let mockTransaction: BraveWallet.TransactionInfo = .previewConfirmedERC20Approve
    let mockTransactions: [BraveWallet.TransactionInfo] = [mockTransaction].map { tx in
      tx.txStatus = .unapproved
      return tx
    }
    let mockGasEstimation: BraveWallet.GasEstimation1559 = .init()
    let store = setupStore(
      accountInfos: [.mockEthAccount],
      allTokens: mockAllTokens,
      transactions: mockTransactions,
      gasEstimation: mockGasEstimation,
      makeErc20ApproveDataSuccess: true,
      setDataForUnapprovedTransactionSuccess: false
    )
    
    let prepareExpectation = expectation(description: "prepare")
    await store.prepare()
    store.$activeTransactionId
      .sink { id in
        defer { prepareExpectation.fulfill() }
        XCTAssertEqual(id, mockTransaction.id)
      }
      .store(in: &cancellables)
    await fulfillment(of: [prepareExpectation], timeout: 1)
    
    let editExpectation = expectation(description: "edit allowance")
    store.editAllowance(
      transaction: mockTransaction,
      spenderAddress: mockTransaction.txArgs[safe: 0] ?? "",
      amount: "0x0",
      completion: { success in
        if !success {
          editExpectation.fulfill()
        }
      }
    )
    await fulfillment(of: [editExpectation], timeout: 1)
  }
  
  /// Test `editAllowance(txMetaId:spenderAddress:amount:completion)` will return true if we suceed in creating and setting ERC20 Approve data with `BraveWalletEthTxManagerProxy`
  func testEditAllowanceSuccess() async {
    let mockAllTokens: [BraveWallet.BlockchainToken] = [.previewToken, .daiToken]
    let mockTransaction: BraveWallet.TransactionInfo = .previewConfirmedERC20Approve
    let mockTransactions: [BraveWallet.TransactionInfo] = [mockTransaction].map { tx in
      tx.txStatus = .unapproved
      return tx
    }
    let mockGasEstimation: BraveWallet.GasEstimation1559 = .init()
    let store = setupStore(
      accountInfos: [.mockEthAccount],
      allTokens: mockAllTokens,
      transactions: mockTransactions,
      gasEstimation: mockGasEstimation,
      makeErc20ApproveDataSuccess: true,
      setDataForUnapprovedTransactionSuccess: true
    )
    
    let prepareExpectation = expectation(description: "prepare")
    await store.prepare()
    store.$activeTransactionId
      .sink { id in
        defer { prepareExpectation.fulfill() }
        XCTAssertEqual(id, mockTransaction.id)
      }
      .store(in: &cancellables)
    await fulfillment(of: [prepareExpectation], timeout: 1)
    
    let editExpectation = expectation(description: "edit allowance")
    store.editAllowance(
      transaction: mockTransaction,
      spenderAddress: mockTransaction.txArgs[safe: 0] ?? "",
      amount: "0x0",
      completion: { success in
        if success {
          editExpectation.fulfill()
        }
      }
    )
    await fulfillment(of: [editExpectation], timeout: 1)
  }
}

private extension BraveWallet.BlockchainToken {
  
  /// DAI token with contract address matching `BraveWallet.TransactionInfo.previewConfirmedERC20Approve`
  static let daiToken: BraveWallet.BlockchainToken = .init(
    contractAddress: "0xad6d458402f60fd3bd25163575031acdce07538d",
    name: "DAI",
    logo: "",
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    isNft: false,
    symbol: "DAI",
    decimals: 18,
    visible: false,
    tokenId: "",
    coingeckoId: "",
    chainId: "",
    coin: .eth
  )
}

extension XCTestCase {
  @MainActor func fulfillment(of expectations: [XCTestExpectation], timeout: TimeInterval) async {
    #if compiler(>=5.8)
    await fulfillment(of: expectations, timeout: timeout, enforceOrder: false)
    #else
    await withCheckedContinuation { continuation in
      waitForExpectations(timeout: timeout) { error in
        // `fulfillment(of:timeout:enforceOrder:) timeout will fail on the function
        // Verify we did not timeout here
        XCTAssertNil(error)
        continuation.resume()
      }
    }
    #endif
  }
}
