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
  let solTestnetBalance: UInt64 = 1_000_000_000  // 1 SOL
  let solTestnetDecimalBalance: Double = 1  // 1 SOL

  private func setupServices(
    mockEthBalanceWei: String = "",
    mockERC20BalanceWei: String = "",
    mockNFTBalances: [BraveWallet.NftIdentifier: Double] = [:],
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

    let rpcService = MockJsonRpcService()
    rpcService.hiddenNetworks.removeAll()
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
    rpcService._nftBalances = { _, nftIdentifiers, _, completion in
      var balances: [NSNumber] = []
      for nft in nftIdentifiers {
        var balance: Double = 0
        if let firstId = mockNFTBalances.keys.first(where: { id in
          id.contractAddress.caseInsensitiveCompare(nft.contractAddress) == .orderedSame
            && id.chainId == nft.chainId
            && id.tokenId == nft.tokenId
        }) {
          balance = mockNFTBalances[firstId] ?? 0
        }
        balances.append(balance as NSNumber)
      }
      completion(balances, "")
    }
    rpcService._solanaBalance = { accountAddress, chainId, completion in
      if chainId == BraveWallet.SolanaMainnet {
        completion(mockLamportBalance, .success, "")
      } else {  // testnet balance
        completion(self.solTestnetBalance, .success, "")
      }
    }
    rpcService._splTokenAccountBalance = { _, tokenMintAddress, _, completion in
      // spd token
      completion(
        mockSplTokenBalances[tokenMintAddress] ?? "",
        UInt8(0),
        "",
        .success,
        ""
      )
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
    let feeEstimation = BraveWallet.SolanaFeeEstimation(
      baseFee: UInt64(0),
      computeUnits: UInt32(0),
      feePerComputeUnit: UInt64(0)
    )
    solTxManagerProxy._solanaTxFeeEstimation = { $2(feeEstimation, .success, "") }

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
    let formatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))
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
    let mockERC721Balance: Double = 1

    let mockERC721Metadata: BraveWallet.NftMetadata = .init(
      name: "mock nft name",
      description: "mock nft description",
      image: "mock.image.url",
      imageData: "mock.image.data",
      externalUrl: "mock.external.url",
      attributes: [],
      backgroundColor: "mock.backgroundColor",
      animationUrl: "mock.animation.url",
      youtubeUrl: "mock.youtube.url",
      collection: "mock.collection"
    )

    // default in mainnet
    let ethSendTxCopy =
      BraveWallet.TransactionInfo.previewConfirmedSend.copy() as! BraveWallet.TransactionInfo
    let sepoliaSwapTxCopy =
      BraveWallet.TransactionInfo.previewConfirmedSwap.copy() as! BraveWallet.TransactionInfo
    sepoliaSwapTxCopy.chainId = BraveWallet.SepoliaChainId

    let mockERC721NFT = BraveWallet.BlockchainToken.mockERC721NFTToken.copy(asVisibleAsset: true)
    let mockERC721NFTTokenIdentifier: BraveWallet.NftIdentifier = .init(
      chainId: mockERC721NFT.chainId,
      contractAddress: mockERC721NFT.contractAddress,
      tokenId: mockERC721NFT.tokenId
    )
    let (
      keyringService, rpcService, walletService, blockchainRegistry, assetRatioService,
      swapService, txService, solTxManagerProxy, ipfsApi, bitcoinWalletService
    ) = setupServices(
      mockEthBalanceWei: mockEthBalanceWei,
      mockERC20BalanceWei: mockERC20BalanceWei,
      mockNFTBalances: [mockERC721NFTTokenIdentifier: mockERC721Balance],
      transactions: [sepoliaSwapTxCopy, ethSendTxCopy].enumerated().map { (index, tx) in
        // transactions sorted by created time, make sure they are in-order
        tx.createdTime = firstTransactionDate.addingTimeInterval(TimeInterval(index) * 1.days)
        return tx
      }
    )

    rpcService._nftMetadatas = { _, _, completion in
      completion([mockERC721Metadata], "")
    }
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      [
        NetworkAssets(
          network: .mockMainnet,
          tokens: [
            BraveWallet.NetworkInfo.mockMainnet.nativeToken.copy(asVisibleAsset: true),
            mockERC721NFT,
            .mockUSDCToken.copy(asVisibleAsset: true),
            // To verify brave/brave-browser#36806
            .previewDaiToken.copy(asVisibleAsset: false),
          ],
          sortOrder: 0
        ),
        NetworkAssets(
          network: .mockSepolia,
          tokens: [BraveWallet.NetworkInfo.mockSepolia.nativeToken.copy(asVisibleAsset: true)],
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
        XCTAssertEqual(lastUpdatedAssets.count, 2)

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

        // 0 balance asset is hidden
        XCTAssertNil(lastUpdatedAssets[safe: 2])

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
          Int(mockERC721Balance)
        )
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.nftMetadata?.image,
          mockERC721Metadata.image
        )
        XCTAssertEqual(lastUpdatedNFTs[safe: 0]?.nftMetadata?.name, mockERC721Metadata.name)
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.nftMetadata?.desc,
          mockERC721Metadata.desc
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
        XCTAssertEqual(secondSectionTxs[safe: 0]?.transaction, sepoliaSwapTxCopy)
        XCTAssertEqual(secondSectionTxs[safe: 0]?.transaction.chainId, sepoliaSwapTxCopy.chainId)
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

    let mockSpdTokenBalance: Double = 1_000_000_000  // 1000
    let mockSpdTokenDecimalBalance: Double = 1000
    let mockSolanaNFTTokenBalance: Double = 1

    let mockSplTokenBalances: [String: String] = [
      BraveWallet.BlockchainToken.mockSpdToken.contractAddress: "\(mockSpdTokenBalance)"
    ]

    let mockSolMetadata: BraveWallet.NftMetadata = .init(
      name: "sol mock nft name",
      description: "sol mock nft description",
      image: "sol.mock.image.url",
      imageData: "sol.mock.image.data",
      externalUrl: "sol.mock.external.url",
      attributes: [],
      backgroundColor: "sol.mock.backgroundColor",
      animationUrl: "sol.mock.animation.url",
      youtubeUrl: "sol.mock.youtube.url",
      collection: "sol.mock.collection"
    )

    let solSendTxCopy =
      BraveWallet.TransactionInfo.previewConfirmedSolSystemTransfer.copy()
      as! BraveWallet.TransactionInfo  // default in mainnet
    let solTestnetSendTxCopy =
      BraveWallet.TransactionInfo.previewConfirmedSolTokenTransfer.copy()
      as! BraveWallet.TransactionInfo
    solTestnetSendTxCopy.chainId = BraveWallet.SolanaTestnet

    let mockSolanaNFTToken =
      BraveWallet.BlockchainToken.mockSolanaNFTToken.copy(asVisibleAsset: true)
    let mockSolanaNFTTokenIdentifier = BraveWallet.NftIdentifier(
      chainId: mockSolanaNFTToken.chainId,
      contractAddress: mockSolanaNFTToken.contractAddress,
      tokenId: mockSolanaNFTToken.tokenId
    )

    let (
      keyringService, rpcService, walletService, blockchainRegistry, assetRatioService,
      swapService, txService, solTxManagerProxy, ipfsApi, bitcoinWalletService
    ) = setupServices(
      mockNFTBalances: [mockSolanaNFTTokenIdentifier: mockSolanaNFTTokenBalance],
      mockLamportBalance: mockLamportBalance,
      mockSplTokenBalances: mockSplTokenBalances,
      transactions: [solTestnetSendTxCopy, solSendTxCopy].enumerated().map { (index, tx) in
        // transactions sorted by created time, make sure they are in-order
        tx.createdTime = firstTransactionDate.addingTimeInterval(TimeInterval(index) * 1.days)
        return tx
      }
    )

    rpcService._nftMetadatas = { _, _, completion in
      completion([mockSolMetadata], "")
    }
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      [
        NetworkAssets(
          network: .mockSolana,
          tokens: [
            BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
            mockSolanaNFTToken,
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
          BraveWallet.BlockchainToken.mockSpdToken.symbol
        )
        XCTAssertEqual(lastUpdatedAssets[safe: 0]?.network, BraveWallet.NetworkInfo.mockSolana)
        XCTAssertEqual(lastUpdatedAssets[safe: 0]?.totalBalance, mockSpdTokenDecimalBalance)
        XCTAssertEqual(
          lastUpdatedAssets[safe: 0]?.price,
          self.mockAssetPrices[safe: 3]?.price ?? ""
        )

        XCTAssertEqual(
          lastUpdatedAssets[safe: 1]?.token.symbol,
          BraveWallet.NetworkInfo.mockSolana.nativeToken.symbol
        )
        XCTAssertEqual(lastUpdatedAssets[safe: 1]?.network, BraveWallet.NetworkInfo.mockSolana)
        XCTAssertEqual(lastUpdatedAssets[safe: 1]?.totalBalance, mockSolDecimalBalance)
        XCTAssertEqual(
          lastUpdatedAssets[safe: 1]?.price,
          self.mockAssetPrices[safe: 2]?.price ?? ""
        )

        XCTAssertEqual(
          lastUpdatedAssets[safe: 2]?.token.symbol,
          BraveWallet.NetworkInfo.mockSolana.nativeToken.symbol
        )
        XCTAssertEqual(
          lastUpdatedAssets[safe: 2]?.network,
          BraveWallet.NetworkInfo.mockSolanaTestnet
        )
        XCTAssertEqual(lastUpdatedAssets[safe: 2]?.totalBalance, self.solTestnetDecimalBalance)
        XCTAssertEqual(
          lastUpdatedAssets[safe: 2]?.price,
          self.mockAssetPrices[safe: 2]?.price ?? ""
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
          lastUpdatedNFTs[safe: 0]?.nftMetadata?.image,
          mockSolMetadata.image
        )
        XCTAssertEqual(lastUpdatedNFTs[safe: 0]?.nftMetadata?.name, mockSolMetadata.name)
        XCTAssertEqual(
          lastUpdatedNFTs[safe: 0]?.nftMetadata?.desc,
          mockSolMetadata.desc
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
      isRetriable: false,
      swapInfo: nil
    )

    let transactionCopy = transaction.copy() as! BraveWallet.TransactionInfo
    transactionCopy.id = UUID().uuidString
    transactionCopy.chainId = BraveWallet.FilecoinTestnet

    let formatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))
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
