// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import XCTest

@testable import BraveWallet

class TransactionDetailsStoreTests: XCTestCase {

  private var cancellables: Set<AnyCancellable> = []

  private func setupServices() -> (
    BraveWalletKeyringService,
    BraveWalletBraveWalletService,
    BraveWalletJsonRpcService,
    BraveWalletAssetRatioService,
    BraveWalletBlockchainRegistry,
    BraveWalletTxService,
    BraveWalletSolanaTxManagerProxy,
    IpfsAPI,
    WalletUserAssetManagerType
  ) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._allAccounts = { completion in
      let allAccounts = [BraveWallet.AccountInfo.previewAccount]
      let allAccountsInfo: BraveWallet.AllAccountsInfo = .init(
        accounts: allAccounts,
        selectedAccount: allAccounts.first,
        ethDappSelectedAccount: allAccounts.first(where: { $0.coin == .eth }),
        solDappSelectedAccount: allAccounts.first(where: { $0.coin == .sol })
      )
      completion(allAccountsInfo)
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    let rpcService = MockJsonRpcService()
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { _, _, _, completion in
      let mockAssetPrices: [BraveWallet.AssetPrice] = [
        .init(fromAsset: "eth", toAsset: "usd", price: "3059.99", assetTimeframeChange: "-57.23")
      ]
      completion(true, mockAssetPrices)
    }
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._allTokens = { _, _, completion in
      completion([])
    }
    let txService = BraveWallet.TestTxService()
    txService._addObserver = { _ in }
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    let userAssetManager = TestableWalletUserAssetManager()
    userAssetManager._getUserAssets = { _, _ in
      return [
        NetworkAssets(
          network: .mockSepolia,
          tokens: [
            .previewToken.copy(asVisibleAsset: true).then {
              $0.chainId = BraveWallet.SepoliaChainId
            }
          ],
          sortOrder: 0
        )
      ]
    }
    let ipfsApi = TestIpfsAPI()

    return (
      keyringService, walletService, rpcService, assetRatioService, blockchainRegistry, txService,
      solTxManagerProxy, ipfsApi, userAssetManager
    )
  }

  /// Test `update()` will populate `parsedTransaction`
  func testUpdate() {
    let transaction: BraveWallet.TransactionInfo = .previewConfirmedSend.then {
      $0.chainId = BraveWallet.SepoliaChainId
    }
    let (
      keyringService, walletService, rpcService, assetRatioService, blockchainRegistry, txService,
      solTxManagerProxy, ipfsApi, userAssetManager
    ) = setupServices()
    let store = TransactionDetailsStore(
      transaction: transaction,
      parsedTransaction: nil,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      txService: txService,
      solanaTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      userAssetManager: userAssetManager
    )
    let parsedTransactionExpectation = expectation(description: "update-parsedTransaction")
    store.$parsedTransaction
      .dropFirst()
      .collect(2)
      .sink { parsedTransactions in
        guard parsedTransactions.count == 2,
          let parsedTransaction = parsedTransactions.last
        else {
          XCTFail("Expected 2 parsedTransaction updates.")
          return
        }
        defer { parsedTransactionExpectation.fulfill() }
        XCTAssertNotNil(parsedTransaction)
        XCTAssertEqual(parsedTransaction?.transaction.txHash, transaction.txHash)
        // the currencyFormatter is set to have maximumFractionDigits equals to 6
        XCTAssertEqual(parsedTransaction?.gasFee, .init(fee: "0.003402", fiat: "$10.410086"))
      }
      .store(in: &cancellables)
    let networkExpectation = expectation(description: "update-network")
    store.$network
      .dropFirst()
      .first()
      .sink { network in
        defer { networkExpectation.fulfill() }
        XCTAssertEqual(network, .mockSepolia)
      }
      .store(in: &cancellables)
    store.update()
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `updateTransaction(_:)` will update `parsedTransaction` with the new transaction
  func testUpdateTransaction() {
    let submittedTx: BraveWallet.TransactionInfo = .previewConfirmedSend.then {
      $0.chainId = BraveWallet.SepoliaChainId
      $0.txStatus = .submitted
    }
    let confirmedTx: BraveWallet.TransactionInfo = .previewConfirmedSend.then {
      $0.chainId = BraveWallet.SepoliaChainId
      $0.txStatus = .confirmed
    }
    XCTAssertEqual(submittedTx.txHash, confirmedTx.txHash)
    let (
      keyringService, walletService, rpcService, assetRatioService, blockchainRegistry, txService,
      solTxManagerProxy, ipfsApi, userAssetManager
    ) = setupServices()
    let store = TransactionDetailsStore(
      transaction: submittedTx,
      parsedTransaction: nil,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      txService: txService,
      solanaTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      userAssetManager: userAssetManager
    )
    let parsedTxExpectation = expectation(description: "update-parsedTx")
    store.$parsedTransaction
      .dropFirst()
      .collect(2)
      .sink { parsedTransactions in
        guard parsedTransactions.count == 2,
          let parsedTransaction = parsedTransactions.first
        else {
          XCTFail("Expected 2 parsedTransaction updates.")
          return
        }
        defer { parsedTxExpectation.fulfill() }
        XCTAssertNotNil(parsedTransaction)
        XCTAssertEqual(parsedTransaction?.transaction.txHash, submittedTx.txHash)
        XCTAssertEqual(parsedTransaction?.transaction.txStatus, .submitted)
      }
      .store(in: &cancellables)
    store.update()
    wait(for: [parsedTxExpectation], timeout: 1)
    cancellables.removeAll()
    let confirmedParsedTxExpectation = expectation(description: "update-parsedTx-confirmed")
    store.$parsedTransaction
      .dropFirst()
      .collect(2)
      .sink { parsedTransactions in
        guard parsedTransactions.count == 2,
          let parsedTransaction = parsedTransactions.first
        else {
          XCTFail("Expected 2 parsedTransaction updates.")
          return
        }
        defer { confirmedParsedTxExpectation.fulfill() }
        XCTAssertNotNil(parsedTransaction)
        XCTAssertEqual(parsedTransaction?.transaction.txHash, confirmedTx.txHash)
        XCTAssertEqual(parsedTransaction?.transaction.txStatus, .confirmed)
      }
      .store(in: &cancellables)
    store.updateTransaction(confirmedTx)
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
}
