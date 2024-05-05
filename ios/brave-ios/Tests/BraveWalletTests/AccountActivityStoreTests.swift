// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Preferences
import XCTest

@testable import BraveWallet

class AccountActivityStoreTests: XCTestCase {

  private var cancellables: Set<AnyCancellable> = .init()

  let networks: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = [
    .eth: [.mockMainnet, .mockGoerli],
    .sol: [.mockSolana, .mockSolanaTestnet],
    .fil: [.mockFilecoinMainnet, .mockFilecoinTestnet],
  ]
  let tokenRegistry: [BraveWallet.CoinType: [BraveWallet.BlockchainToken]] = [:]
  let mockAssetPrices: [BraveWallet.AssetPrice] = [
    .init(fromAsset: "eth", toAsset: "usd", price: "3059.99", assetTimeframeChange: "-57.23"),
    .init(
      fromAsset: BraveWallet.BlockchainToken.mockUSDCToken.assetRatioId,
      toAsset: "usd",
      price: "1.00",
      assetTimeframeChange: "-57.23"
    ),
    .init(fromAsset: "sol", toAsset: "usd", price: "2.00", assetTimeframeChange: "-57.23"),
    .init(
      fromAsset: BraveWallet.BlockchainToken.mockSpdToken.assetRatioId.lowercased(),
      toAsset: "usd",
      price: "0.50",
      assetTimeframeChange: "-57.23"
    ),
    .init(
      fromAsset: BraveWallet.BlockchainToken.mockFilToken.assetRatioId.lowercased(),
      toAsset: "usd",
      price: "4.00",
      assetTimeframeChange: "-57.23"
    ),
  ]

  private func setupServices(
    mockEthBalanceWei: String = "",
    mockERC20BalanceWei: String = "",
    mockERC721BalanceWei: String = "",
    mockLamportBalance: UInt64 = 0,
    mockSplTokenBalances: [String: String] = [:],  // [tokenMintAddress: balance],
    mockFilBalance: String = "",
    mockFilTestnetBalance: String = "",
    transactions: [BraveWallet.TransactionInfo]
  ) -> (
    BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService,
    BraveWallet.TestBraveWalletService, BraveWallet.TestBlockchainRegistry,
    BraveWallet.TestAssetRatioService, BraveWallet.TestSwapService,
    BraveWallet.TestTxService, BraveWallet.TestSolanaTxManagerProxy,
    IpfsAPI,
    BraveWallet.TestBitcoinWalletService
  ) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._allAccounts = {
      $0(.mock)
    }

    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._allNetworks = { coin, completion in
      completion(self.networks[coin] ?? [])
    }
    rpcService._balance = { _, coin, chainId, completion in
      switch chainId {
      case BraveWallet.MainnetChainId:
        completion(mockEthBalanceWei, .success, "")
      case BraveWallet.FilecoinMainnet:
        completion(mockFilBalance, .success, "")
      case BraveWallet.FilecoinTestnet:
        completion(mockFilTestnetBalance, .success, "")
      default:
        completion("", .internalError, "")
      }
    }
    rpcService._erc20TokenBalance = { _, _, _, completion in
      completion(mockERC20BalanceWei, .success, "")
    }
    rpcService._erc721TokenBalance = { _, _, _, _, completion in
      completion(mockERC721BalanceWei, .success, "")  // eth nft balance
    }
    rpcService._solanaBalance = { accountAddress, chainId, completion in
      if chainId == BraveWallet.SolanaMainnet {
        completion(mockLamportBalance, .success, "")
      } else {  // testnet balance
        completion(0, .success, "")
      }
    }
    rpcService._splTokenAccountBalance = { _, tokenMintAddress, _, completion in
      // spd token, sol nft balance
      completion(
        mockSplTokenBalances[tokenMintAddress] ?? "",
        UInt8(0),
        mockSplTokenBalances[tokenMintAddress] ?? "",
        .success,
        ""
      )
    }
    rpcService._erc721Metadata = { _, _, _, completion in
      let metadata = """
        {
          "image": "mock.image.url",
          "name": "mock nft name",
          "description": "mock nft description"
        }
        """
      completion("", metadata, .success, "")
    }
    rpcService._solTokenMetadata = { _, _, completion in
      let metaData = """
        {
          "image": "sol.mock.image.url",
          "name": "sol mock nft name",
          "description": "sol mock nft description"
        }
        """
      completion("", metaData, .success, "")
    }
    rpcService._hiddenNetworks = {
      $1([])
    }

    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }

    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._allTokens = { chainId, coin, completion in
      completion(self.tokenRegistry[coin] ?? [])
    }
    blockchainRegistry._buyTokens = { _, chainId, completion in
      let tokensForChainId = self.tokenRegistry
        .flatMap(\.value)
        .filter { $0.chainId == chainId }
      completion(tokensForChainId)
    }

    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { _, _, _, completion in
      completion(true, self.mockAssetPrices)
    }

    let swapService = BraveWallet.TestSwapService()
    swapService._isSwapSupported = { chainId, completion in
      let isSupported =
        chainId == BraveWallet.MainnetChainId
        || chainId == BraveWallet.SolanaMainnet
      completion(isSupported)
    }

    let txService = BraveWallet.TestTxService()
    txService._addObserver = { _ in }
    txService._allTransactionInfo = { coin, chainId, _, completion in
      completion(transactions.filter({ $0.chainId == chainId }))
    }

    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._estimatedTxFee = { $2(0, .success, "") }

    let ipfsApi = TestIpfsAPI()

    let bitcoinWalletService = BraveWallet.TestBitcoinWalletService()
    bitcoinWalletService._balance = {
      $1(.init(totalBalance: 1000, availableBalance: 1000, pendingBalance: 0, balances: [:]), nil)
    }

    return (
      keyringService, rpcService, walletService, blockchainRegistry, assetRatioService,
      swapService, txService, solTxManagerProxy, ipfsApi, bitcoinWalletService
    )
  }

  func testUpdateEthereumAccount() {
    // Monday, November 8, 2021 7:27:51 PM
    let firstTransactionDate = Date(timeIntervalSince1970: 1_636_399_671)
    let account: BraveWallet.AccountInfo = .mockEthAccount
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    let mockEthDecimalBalance: Double = 0.0896
    let numEthDecimals = Int(BraveWallet.NetworkInfo.mockMainnet.nativeToken.decimals)
    let mockEthBalanceWei =
      formatter.weiString(from: mockEthDecimalBalance, radix: .hex, decimals: numEthDecimals) ?? ""
    let mockERC20DecimalBalance = 1.5
    let mockERC20BalanceWei =
      formatter.weiString(
        from: mockERC20DecimalBalance,
        radix: .hex,
        decimals: Int(BraveWallet.BlockchainToken.mockUSDCToken.decimals)
      ) ?? ""
    let mockNFTBalance: Double = 1
    let mockERC721BalanceWei =
      formatter.weiString(from: mockNFTBalance, radix: .hex, decimals: 0) ?? ""

    let mockERC721Metadata: NFTMetadata = .init(
      imageURLString: "mock.image.url",
      name: "mock nft name",
      description: "mock nft description",
      attributes: nil
    )

    // default in mainnet
    let ethSendTxCopy =
      BraveWallet.TransactionInfo.previewConfirmedSend.copy() as! BraveWallet.TransactionInfo
    let goerliSwapTxCopy =
      BraveWallet.TransactionInfo.previewConfirmedSwap.copy() as! BraveWallet.TransactionInfo
    goerliSwapTxCopy.chainId = BraveWallet.GoerliChainId

    let (
      keyringService, rpcService, walletService, blockchainRegistry, assetRatioService,
      swapService, txService, solTxManagerProxy, ipfsApi, bitcoinWalletService
    ) = setupServices(
      mockEthBalanceWei: mockEthBalanceWei,
      mockERC20BalanceWei: mockERC20BalanceWei,
      mockERC721BalanceWei: mockERC721BalanceWei,
      transactions: [goerliSwapTxCopy, ethSendTxCopy].enumerated().map { (index, tx) in
        // transactions sorted by created time, make sure they are in-order
        tx.createdTime = firstTransactionDate.addingTimeInterval(TimeInterval(index) * 1.days)
        return tx
      }
    )

    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      [
        NetworkAssets(
          network: .mockMainnet,
          tokens: [
            BraveWallet.NetworkInfo.mockMainnet.nativeToken.copy(asVisibleAsset: true),
            .mockERC721NFTToken.copy(asVisibleAsset: true),
            .mockUSDCToken.copy(asVisibleAsset: true),
            // To verify brave/brave-browser#36806
            .previewDaiToken.copy(asVisibleAsset: false),
          ],
          sortOrder: 0
        ),
        NetworkAssets(
          network: .mockGoerli,
          tokens: [BraveWallet.NetworkInfo.mockGoerli.nativeToken.copy(asVisibleAsset: true)],
          sortOrder: 1
        ),
      ]
    }

    let accountActivityStore = AccountActivityStore(
      account: account,
      isWalletPanel: false,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      swapService: swapService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      bitcoinWalletService: bitcoinWalletService,
      userAssetManager: mockAssetManager
    )

    let userAssetsException = expectation(description: "accountActivityStore-userAssets")
    accountActivityStore.$userAssets
      .dropFirst()
      .collect(3)
      .sink { userAssets in
        defer { userAssetsException.fulfill() }
        guard let lastUpdatedAssets = userAssets.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssets.count, 3)

        XCTAssertEqual(
          lastUpdatedAssets[0].token.symbol,
          BraveWallet.NetworkInfo.mockMainnet.nativeToken.symbol
        )
        XCTAssertEqual(lastUpdatedAssets[0].network, BraveWallet.NetworkInfo.mockMainnet)
        XCTAssertEqual(lastUpdatedAssets[0].totalBalance, mockEthDecimalBalance)
        XCTAssertEqual(lastUpdatedAssets[0].price, self.mockAssetPrices[safe: 0]?.price ?? "")

        XCTAssertEqual(
          lastUpdatedAssets[1].token.symbol,
          BraveWallet.BlockchainToken.mockUSDCToken.symbol
        )
        XCTAssertEqual(lastUpdatedAssets[1].network, BraveWallet.NetworkInfo.mockMainnet)
        XCTAssertEqual(lastUpdatedAssets[1].totalBalance, mockERC20DecimalBalance)
        XCTAssertEqual(lastUpdatedAssets[1].price, self.mockAssetPrices[safe: 1]?.price ?? "")

        XCTAssertEqual(
          lastUpdatedAssets[2].token.symbol,
          BraveWallet.NetworkInfo.mockGoerli.nativeToken.symbol
        )
        XCTAssertEqual(lastUpdatedAssets[2].network, BraveWallet.NetworkInfo.mockGoerli)
        XCTAssertEqual(lastUpdatedAssets[2].totalBalance, 0)
        XCTAssertEqual(lastUpdatedAssets[2].price, self.mockAssetPrices[safe: 0]?.price ?? "")

        // Verify brave/brave-browser#36806
        let daiTokenVisible = lastUpdatedAssets.contains(where: {
          $0.id == BraveWallet.BlockchainToken.previewDaiToken.id
        })
        XCTAssertFalse(daiTokenVisible)
      }
      .store(in: &cancellables)

    let userNFTsException = expectation(description: "accountActivityStore-userNFTs")
    XCTAssertTrue(accountActivityStore.userNFTs.isEmpty)  // Initial state
    accountActivityStore.$userNFTs
      .dropFirst()
      .collect(3)
      .sink { userNFTs in
        defer { userNFTsException.fulfill() }
        guard let lastUpdatedNFTs = userNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedNFTs.count, 1)
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockERC721NFTToken.symbol
        )
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.balanceForAccounts[account.id],
          Int(mockNFTBalance)
        )
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.nftMetadata?.imageURLString,
          mockERC721Metadata.imageURLString
        )
        XCTAssertEqual(lastUpdatedNFTs[safe: 0]?.nftMetadata?.name, mockERC721Metadata.name)
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.nftMetadata?.description,
          mockERC721Metadata.description
        )
      }.store(in: &cancellables)

    let transactionSectionsExpectation = expectation(
      description: "accountActivityStore-transactions"
    )
    XCTAssertTrue(accountActivityStore.transactionSections.isEmpty)
    accountActivityStore.$transactionSections
      .dropFirst()
      .collect(3)
      .sink { transactionSectionsCollected in
        defer { transactionSectionsExpectation.fulfill() }
        guard let transactionSections = transactionSectionsCollected.last else {
          XCTFail("Unexpected test result")
          return
        }
        // `ParsedTransaction`s are tested in `TransactionParserTests`,
        // just verify they are populated with correct tx
        XCTAssertEqual(transactionSections.count, 2)
        let firstSectionTxs = transactionSections[safe: 0]?.transactions ?? []
        XCTAssertEqual(firstSectionTxs[safe: 0]?.transaction, ethSendTxCopy)
        XCTAssertEqual(firstSectionTxs[safe: 0]?.transaction.chainId, ethSendTxCopy.chainId)
        let secondSectionTxs = transactionSections[safe: 1]?.transactions ?? []
        XCTAssertEqual(secondSectionTxs[safe: 0]?.transaction, goerliSwapTxCopy)
        XCTAssertEqual(secondSectionTxs[safe: 0]?.transaction.chainId, goerliSwapTxCopy.chainId)
      }.store(in: &cancellables)

    accountActivityStore.update()

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  func testUpdateSolanaAccount() {
    // Monday, November 8, 2021 7:27:51 PM
    let firstTransactionDate = Date(timeIntervalSince1970: 1_636_399_671)
    let account: BraveWallet.AccountInfo = .mockSolAccount
    let mockLamportBalance: UInt64 = 3_876_535_000  // ~3.8765 SOL
    let mockSolDecimalBalance: Double = 3.8765  // rounded

    let mockSpdTokenBalance: Double = 0
    let mockSolanaNFTTokenBalance: Double = 1

    let mockSplTokenBalances: [String: String] = [
      BraveWallet.BlockchainToken.mockSpdToken.contractAddress: "\(mockSpdTokenBalance)",
      BraveWallet.BlockchainToken.mockSolanaNFTToken.contractAddress:
        "\(mockSolanaNFTTokenBalance)",
    ]

    let mockSolMetadata: NFTMetadata = .init(
      imageURLString: "sol.mock.image.url",
      name: "sol mock nft name",
      description: "sol mock nft description",
      attributes: nil
    )

    let solSendTxCopy =
      BraveWallet.TransactionInfo.previewConfirmedSolSystemTransfer.copy()
      as! BraveWallet.TransactionInfo  // default in mainnet
    let solTestnetSendTxCopy =
      BraveWallet.TransactionInfo.previewConfirmedSolTokenTransfer.copy()
      as! BraveWallet.TransactionInfo
    solTestnetSendTxCopy.chainId = BraveWallet.SolanaTestnet

    let (
      keyringService, rpcService, walletService, blockchainRegistry, assetRatioService,
      swapService, txService, solTxManagerProxy, ipfsApi, bitcoinWalletService
    ) = setupServices(
      mockLamportBalance: mockLamportBalance,
      mockSplTokenBalances: mockSplTokenBalances,
      transactions: [solTestnetSendTxCopy, solSendTxCopy].enumerated().map { (index, tx) in
        // transactions sorted by created time, make sure they are in-order
        tx.createdTime = firstTransactionDate.addingTimeInterval(TimeInterval(index) * 1.days)
        return tx
      }
    )

    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      [
        NetworkAssets(
          network: .mockSolana,
          tokens: [
            BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
            .mockSolanaNFTToken.copy(asVisibleAsset: true),
            .mockSpdToken.copy(asVisibleAsset: true),
          ],
          sortOrder: 0
        ),
        NetworkAssets(
          network: .mockSolanaTestnet,
          tokens: [
            BraveWallet.NetworkInfo.mockSolanaTestnet.nativeToken.copy(asVisibleAsset: true)
          ],
          sortOrder: 1
        ),
      ]
    }

    let accountActivityStore = AccountActivityStore(
      account: account,
      isWalletPanel: false,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      swapService: swapService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      bitcoinWalletService: bitcoinWalletService,
      userAssetManager: mockAssetManager
    )

    let userAssetsExpectation = expectation(description: "accountActivityStore-userAssets")
    accountActivityStore.$userAssets
      .dropFirst()
      .collect(3)
      .sink { userAssets in
        defer { userAssetsExpectation.fulfill() }
        guard let lastUpdatedAssets = userAssets.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssets.count, 3)

        XCTAssertEqual(
          lastUpdatedAssets[safe: 0]?.token.symbol,
          BraveWallet.NetworkInfo.mockSolana.nativeToken.symbol
        )
        XCTAssertEqual(lastUpdatedAssets[safe: 0]?.network, BraveWallet.NetworkInfo.mockSolana)
        XCTAssertEqual(lastUpdatedAssets[safe: 0]?.totalBalance, mockSolDecimalBalance)
        XCTAssertEqual(
          lastUpdatedAssets[safe: 0]?.price,
          self.mockAssetPrices[safe: 2]?.price ?? ""
        )

        XCTAssertEqual(
          lastUpdatedAssets[safe: 1]?.token.symbol,
          BraveWallet.BlockchainToken.mockSpdToken.symbol
        )
        XCTAssertEqual(lastUpdatedAssets[safe: 1]?.network, BraveWallet.NetworkInfo.mockSolana)
        XCTAssertEqual(lastUpdatedAssets[safe: 1]?.totalBalance, mockSpdTokenBalance)
        XCTAssertEqual(
          lastUpdatedAssets[safe: 1]?.price,
          self.mockAssetPrices[safe: 3]?.price ?? ""
        )
      }
      .store(in: &cancellables)

    let userNFTsExpectation = expectation(description: "accountActivityStore-userNFTs")
    XCTAssertTrue(accountActivityStore.userNFTs.isEmpty)  // Initial state
    accountActivityStore.$userNFTs
      .dropFirst()
      .collect(3)
      .sink { userNFTs in
        defer { userNFTsExpectation.fulfill() }
        guard let lastUpdatedNFTs = userNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedNFTs.count, 1)
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolanaNFTToken.symbol
        )
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.balanceForAccounts[account.id],
          Int(mockSolanaNFTTokenBalance)
        )
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.nftMetadata?.imageURLString,
          mockSolMetadata.imageURLString
        )
        XCTAssertEqual(lastUpdatedNFTs[safe: 0]?.nftMetadata?.name, mockSolMetadata.name)
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.nftMetadata?.description,
          mockSolMetadata.description
        )
      }.store(in: &cancellables)

    let transactionSectionsExpectation = expectation(
      description: "accountActivityStore-transactions"
    )
    XCTAssertTrue(accountActivityStore.transactionSections.isEmpty)
    accountActivityStore.$transactionSections
      .dropFirst()
      .collect(3)
      .sink { transactionSectionsCollected in
        defer { transactionSectionsExpectation.fulfill() }
        guard let transactionSections = transactionSectionsCollected.last else {
          XCTFail("Unexpected test result")
          return
        }
        // `ParsedTransaction`s are tested in `TransactionParserTests`,
        // just verify they are populated with correct tx
        XCTAssertEqual(transactionSections.count, 2)
        let firstSectionTxs = transactionSections[safe: 0]?.transactions ?? []
        XCTAssertEqual(firstSectionTxs[safe: 0]?.transaction, solSendTxCopy)
        XCTAssertEqual(firstSectionTxs[safe: 0]?.transaction.chainId, solSendTxCopy.chainId)
        let secondSectionTxs = transactionSections[safe: 1]?.transactions ?? []
        XCTAssertEqual(secondSectionTxs[safe: 0]?.transaction, solTestnetSendTxCopy)
        XCTAssertEqual(secondSectionTxs[safe: 0]?.transaction.chainId, solTestnetSendTxCopy.chainId)
      }.store(in: &cancellables)

    accountActivityStore.update()

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  func testUpdateFilecoinAccount() {
    // Monday, November 8, 2021 7:27:51 PM
    let firstTransactionDate = Date(timeIntervalSince1970: 1_636_399_671)
    let account: BraveWallet.AccountInfo = .mockFilAccount

    let transactionData: BraveWallet.FilTxData = .init(
      nonce: "",
      gasPremium: "100911",
      gasFeeCap: "101965",
      gasLimit: "1527953",
      maxFee: "0",
      to: "t1xqhfiydm2yq6augugonr4zpdllh77iw53aexztb",
      value: "1000000000000000000"
    )
    let transaction = BraveWallet.TransactionInfo(
      id: UUID().uuidString,
      fromAddress: "t165quq7gkjh6ebshr7qi2ud7vycel4m7x6dvfvgb",
      from: account.accountId,
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffggggg1234",
      txDataUnion: .init(filTxData: transactionData),
      txStatus: .unapproved,
      txType: .other,
      txParams: [],
      txArgs: [],
      createdTime: firstTransactionDate,
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil,
      chainId: BraveWallet.FilecoinMainnet,
      effectiveRecipient: nil,
      isRetriable: false
    )

    let transactionCopy = transaction.copy() as! BraveWallet.TransactionInfo
    transactionCopy.id = UUID().uuidString
    transactionCopy.chainId = BraveWallet.FilecoinTestnet

    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    let mockFilDecimalBalance: Double = 2
    let filecoinMainnetDecimals = Int(BraveWallet.NetworkInfo.mockFilecoinMainnet.decimals)
    let mockFilDecimalBalanceInWei =
      formatter.weiString(
        from: "\(mockFilDecimalBalance)",
        radix: .decimal,
        decimals: filecoinMainnetDecimals
      ) ?? ""
    let mockFileTestnetDecimalBalance: Double = 1
    let filecoinTestnetDecimals = Int(BraveWallet.NetworkInfo.mockFilecoinTestnet.decimals)
    let mockFilTestnetDecimalBalanceInWei =
      formatter.weiString(
        from: "\(mockFileTestnetDecimalBalance)",
        radix: .decimal,
        decimals: filecoinTestnetDecimals
      ) ?? ""

    let (
      keyringService, rpcService, walletService, blockchainRegistry, assetRatioService,
      swapService, txService, solTxManagerProxy, ipfsApi, bitcoinWalletService
    ) = setupServices(
      mockFilBalance: mockFilDecimalBalanceInWei,
      mockFilTestnetBalance: mockFilTestnetDecimalBalanceInWei,
      transactions: [transaction, transactionCopy].enumerated().map { (index, tx) in
        // transactions sorted by created time, make sure they are in-order
        tx.createdTime = firstTransactionDate.addingTimeInterval(TimeInterval(index) * 1.days)
        return tx
      }
    )

    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      [
        NetworkAssets(
          network: .mockFilecoinMainnet,
          tokens: [
            BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken.copy(asVisibleAsset: true)
          ],
          sortOrder: 0
        ),
        NetworkAssets(
          network: .mockFilecoinTestnet,
          tokens: [
            BraveWallet.NetworkInfo.mockFilecoinTestnet.nativeToken.copy(asVisibleAsset: true)
          ],
          sortOrder: 1
        ),
      ]
    }

    let accountActivityStore = AccountActivityStore(
      account: account,
      isWalletPanel: false,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      swapService: swapService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      bitcoinWalletService: bitcoinWalletService,
      userAssetManager: mockAssetManager
    )

    let userAssetsExpectation = expectation(description: "accountActivityStore-userAssets")
    accountActivityStore.$userAssets
      .dropFirst()
      .collect(3)
      .sink { userAssets in
        defer { userAssetsExpectation.fulfill() }
        guard let lastUpdatedAssets = userAssets.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssets.count, 2)

        XCTAssertEqual(
          lastUpdatedAssets[safe: 0]?.token.symbol,
          BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken.symbol
        )
        XCTAssertEqual(
          lastUpdatedAssets[safe: 0]?.network,
          BraveWallet.NetworkInfo.mockFilecoinMainnet
        )
        XCTAssertEqual(lastUpdatedAssets[safe: 0]?.totalBalance, mockFilDecimalBalance)
        XCTAssertEqual(
          lastUpdatedAssets[safe: 0]?.price,
          self.mockAssetPrices[safe: 4]?.price ?? ""
        )

        XCTAssertEqual(
          lastUpdatedAssets[safe: 1]?.token.symbol,
          BraveWallet.NetworkInfo.mockFilecoinTestnet.nativeToken.symbol
        )
        XCTAssertEqual(
          lastUpdatedAssets[safe: 1]?.network,
          BraveWallet.NetworkInfo.mockFilecoinTestnet
        )
        XCTAssertEqual(lastUpdatedAssets[safe: 1]?.totalBalance, mockFileTestnetDecimalBalance)
        XCTAssertEqual(
          lastUpdatedAssets[safe: 1]?.price,
          self.mockAssetPrices[safe: 4]?.price ?? ""
        )
      }
      .store(in: &cancellables)

    let transactionSectionsExpectation = expectation(
      description: "accountActivityStore-transactions"
    )
    XCTAssertTrue(accountActivityStore.transactionSections.isEmpty)
    accountActivityStore.$transactionSections
      .dropFirst()
      .collect(3)
      .sink { transactionSectionsCollected in
        defer { transactionSectionsExpectation.fulfill() }
        guard let transactionSections = transactionSectionsCollected.last else {
          XCTFail("Unexpected test result")
          return
        }
        // `ParsedTransaction`s are tested in `TransactionParserTests`,
        // just verify they are populated with correct tx
        XCTAssertEqual(transactionSections.count, 1)
        let firstSectionTxs = transactionSections[safe: 0]?.transactions ?? []
        // should not have `transactionCopy` since it's on testnet but the account is on mainnet
        XCTAssertEqual(firstSectionTxs.count, 1)
        XCTAssertEqual(firstSectionTxs[safe: 0]?.transaction, transaction)
        XCTAssertEqual(firstSectionTxs[safe: 0]?.transaction.chainId, transaction.chainId)
      }.store(in: &cancellables)

    accountActivityStore.update()

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
}
