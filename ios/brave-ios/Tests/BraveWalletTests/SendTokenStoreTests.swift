// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import Combine
import Preferences
import XCTest

@testable import BraveWallet

class SendTokenStoreTests: XCTestCase {
  private var cancellables: Set<AnyCancellable> = []
  private let batSymbol = "BAT"

  private let mockERC721Metadata: BraveWallet.NftMetadata = .init(
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
  private let mockSolMetadata: BraveWallet.NftMetadata = .init(
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

  private func setupServices(
    selectedAccount: BraveWallet.AccountInfo = BraveWallet.AccountInfo.previewAccount,
    userAssets: [BraveWallet.NetworkInfo: [BraveWallet.BlockchainToken]] = [
      .mockMainnet: [.previewToken]
    ],
    selectedCoin: BraveWallet.CoinType = .eth,
    balance: String = "0",
    erc20Balance: String = "0",
    erc721Balance: Double = 0,
    solanaBalance: UInt64 = 0,
    splTokenBalance: String = "0",
    snsGetSolAddr: String = "",
    ensGetEthAddr: String = "",
    unstoppableDomainsGetWalletAddr: String = ""
  ) -> (
    BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService,
    BraveWallet.TestBraveWalletService, BraveWallet.TestEthTxManagerProxy,
    BraveWallet.TestSolanaTxManagerProxy, BraveWallet.TestBitcoinWalletService,
    WalletUserAssetManagerType
  ) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._setSelectedAccount = { _, completion in completion(true) }
    keyringService._checksumEthAddress = { address, completion in completion(address) }
    keyringService._allAccounts = { completion in
      completion(
        .init(
          accounts: [selectedAccount],
          selectedAccount: selectedAccount,
          ethDappSelectedAccount: [selectedAccount].first(where: { $0.coin == .eth }),
          solDappSelectedAccount: [selectedAccount].first(where: { $0.coin == .sol })
        )
      )
    }
    let rpcService = MockJsonRpcService()
    rpcService.hiddenNetworks.removeAll()
    rpcService._balance = { $3(balance, .success, "") }
    rpcService._erc20TokenBalance = { $3(erc20Balance, .success, "") }
    rpcService._nftBalances = { $3([erc721Balance as NSNumber], "") }
    rpcService._solanaBalance = { $2(solanaBalance, .success, "") }
    rpcService._splTokenAccountBalance = { _, _, _, completion in
      completion(splTokenBalance, UInt8(6), "", .success, "")
    }
    rpcService._snsGetSolAddr = { address, completion in
      completion(snsGetSolAddr, .success, "")
    }
    rpcService._ensGetEthAddr = { _, completion in
      completion(ensGetEthAddr, false, .success, "")
    }
    rpcService._setEnsResolveMethod = { _ in }
    rpcService._setEnsOffchainLookupResolveMethod = { _ in }
    rpcService._setSnsResolveMethod = { _ in }
    rpcService._setUnstoppableDomainsResolveMethod = { _ in }
    rpcService._unstoppableDomainsGetWalletAddr = { _, _, completion in
      completion(unstoppableDomainsGetWalletAddr, .success, "")
    }
    rpcService._nftMetadatas = { coin, _, completion in
      if coin == .eth {
        completion([self.mockERC721Metadata], "")
      } else if coin == .sol {
        completion([self.mockSolMetadata], "")
      } else {
        completion([], "Error")
      }
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._isBase58EncodedSolanaPubkey = { _, completion in completion(true) }
    walletService._ensureSelectedAccountForChain = { coin, chainId, completion in
      completion(selectedAccount.accountId)
    }
    walletService._addObserver = { _ in }
    let ethTxManagerProxy = BraveWallet.TestEthTxManagerProxy()
    ethTxManagerProxy._makeErc20TransferData = { _, _, completion in
      completion(true, .init())
    }
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()

    let bitcoinWalletService = BraveWallet.TestBitcoinWalletService()
    bitcoinWalletService._balance = { accountId, completion in
      let bitcoinBalance: BraveWallet.BitcoinBalance = .init(
        totalBalance: 100000,
        availableBalance: 100000,
        pendingBalance: 0,
        balances: [:]
      )
      completion(bitcoinBalance, "")
    }

    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getUserAssets = { _, _ in
      userAssets.map { (network, tokens) in
        NetworkAssets(network: network, tokens: tokens, sortOrder: 0)
      }
    }

    return (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    )
  }

  /// Test given a `prefilledToken` will be assigned to `selectedSendToken`
  func testPrefilledToken() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()

    var store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    XCTAssertNil(store.selectedSendToken)

    store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    let sendTokenExpectation = expectation(description: "update-sendTokenExpectation")
    store.$selectedSendToken
      .dropFirst()
      .first()
      .sink { selectedSendToken in
        defer { sendTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, .previewToken)
      }
      .store(in: &cancellables)
    store.update()
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test that given a `prefilledToken` that is not on the current network, the `SendTokenStore`
  /// will switch networks to the `chainId` of the token before assigning the token.
  func testPrefilledTokenSwitchNetwork() {
    var selectedNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()
    rpcService._network = { coin, _, completion in
      completion(selectedNetwork)
    }
    // simulate network switch when `setNetwork` is called
    rpcService._setNetwork = { chainId, coin, origin, completion in
      // verify network switched to SolanaMainnet
      XCTAssertEqual(chainId, BraveWallet.SolanaMainnet)
      selectedNetwork = coin == .eth ? .mockMainnet : .mockSolana
      completion(true)
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .mockSolToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    let sendTokenExpectation = expectation(description: "update-sendTokenExpectation")
    store.$selectedSendToken
      .dropFirst()
      .first()
      .sink { selectedSendToken in
        defer { sendTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, .mockSolToken)
      }
      .store(in: &cancellables)
    store.update()
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `update()` will update the `selectedSendToken` if nil, and update `selectedSendTokenBalance` with the token balance
  func testUpdate() {
    let mockUserAssets: [BraveWallet.BlockchainToken] = [.previewToken, .previewDaiToken]
    let mockDecimalBalance: Double = 0.0896
    let numDecimals = Int(mockUserAssets[0].decimals)
    let formatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: numDecimals))
    let mockBalanceWei =
      formatter.weiString(from: mockDecimalBalance, radix: .hex, decimals: numDecimals) ?? ""

    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(balance: mockBalanceWei)
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    let selectedSendTokenExpectation = expectation(description: "update-selectedSendToken")
    XCTAssertNil(store.selectedSendToken)  // Initial state
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { selectedSendTokenExpectation.fulfill() }
        guard let selectedSendToken = selectedSendToken else {
          XCTFail("selectedSendToken was nil")
          return
        }
        XCTAssertEqual(
          selectedSendToken.symbol,
          BraveWallet.NetworkInfo.mockSepolia.nativeToken.symbol
        )
      }.store(in: &cancellables)
    let sendTokenBalanceExpectation = expectation(description: "update-sendTokenBalance")
    XCTAssertNil(store.selectedSendTokenBalance)  // Initial state
    store.$selectedSendTokenBalance
      .dropFirst()
      .sink { sendTokenBalance in
        defer { sendTokenBalanceExpectation.fulfill() }
        guard let sendTokenBalance = sendTokenBalance else {
          XCTFail("sendTokenBalance was nil")
          return
        }
        XCTAssertEqual(sendTokenBalance, BDouble(mockDecimalBalance))
      }.store(in: &cancellables)
    store.update()
    waitForExpectations(timeout: 30) { error in
      XCTAssertNil(error)
    }
  }

  /// Test making an ETH EIP 1559 transaction
  func testMakeSendETHEIP1559Transaction() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .previewToken
    let ex = expectation(description: "send-eth-eip1559-transaction")
    store.sendToken(amount: "0.01") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test making a ETH Send transaction
  func testMakeSendETHTransaction() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .previewToken
    let ex = expectation(description: "send-eth-transaction")
    store.sendToken(amount: "0.01") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test making an ERC20 EIP 1559 transaction
  func testMakeSendERC20EIP1559Transaction() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    let token: BraveWallet.BlockchainToken = .init(
      contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      name: "Basic Attention Token",
      logo: "",
      isCompressed: false,
      isErc20: true,
      isErc721: false,
      isErc1155: false,
      splTokenProgram: .unsupported,
      isNft: false,
      isSpam: false,
      symbol: batSymbol,
      decimals: 18,
      visible: true,
      tokenId: "",
      coingeckoId: "",
      chainId: "",
      coin: .eth
    )
    store.selectedSendToken = token

    let ex = expectation(description: "send-bat-eip1559-transaction")
    store.sendToken(amount: "0.01") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test making a Send ERC20 transaction
  func testMakeSendERC20Transaction() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .previewToken
    let ex = expectation(description: "send-bat-transaction")
    store.sendToken(amount: "0.01") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test making ERC721 (NFT) Transaction
  func testMakeSendERC721Transaction() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()
    ethTxManagerProxy._makeErc721TransferFromData = { _, _, _, _, completion in
      completion(true, .init())
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .mockERC721NFTToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .mockERC721NFTToken
    let ex = expectation(description: "send-NFT-transaction")
    store.sendToken(amount: "") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `suggestedAmountTapped(.all)` will not round the amount
  func testSendFullBalanceNoRounding() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, _
    ) =
      setupServices()
    let formatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))
    let mockBalance = "47.156499657504857477"
    let mockBalanceWei = formatter.weiString(from: mockBalance, radix: .hex, decimals: 18) ?? ""

    rpcService._chainIdForOrigin = { $2(BraveWallet.NetworkInfo.mockSepolia.chainId) }
    rpcService._network = { $2(BraveWallet.NetworkInfo.mockSepolia) }
    rpcService._balance = { _, _, _, completion in
      completion(mockBalanceWei, .success, "")
    }
    rpcService._nftMetadatas = { _, _, completion in
      completion([], "Error")
    }

    walletService._userAssets = { $2([.previewToken]) }

    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      [NetworkAssets(network: .mockMainnet, tokens: [.previewToken], sortOrder: 0)]
    }

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .previewToken
    let fetchSelectedTokenBalanceEx = expectation(description: "fetchSelectedTokenBalance")
    store.$selectedSendTokenBalance
      .dropFirst()
      .sink { balance in
        defer { fetchSelectedTokenBalanceEx.fulfill() }
        XCTAssertEqual(balance, BDouble(mockBalance)!)
      }
      .store(in: &cancellables)

    store.update()  // fetch balance
    waitForExpectations(timeout: 3) { error in  // wait for balance to be updated
      XCTAssertNil(error)
    }
    let sendFullBalanceEx = expectation(description: "sendFullBalance")
    store.$sendAmount
      .dropFirst()
      .sink { amount in
        defer { sendFullBalanceEx.fulfill() }
        XCTAssertEqual("\(amount)", "\(mockBalance)")
      }
      .store(in: &cancellables)
    store.suggestedAmountTapped(.all)  // balance fetched, simulate user tapping 100%
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test making Solana System Program Transfer transaction
  func testSolSendSystemProgramTransfer() {
    let mockBalance: UInt64 = 47
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      userAssets: [.mockSolana: [.mockSolToken]],
      selectedCoin: .sol,
      solanaBalance: mockBalance
    )
    solTxManagerProxy._makeSystemProgramTransferTxData = { _, _, _, completion in
      completion(.init(), .success, "")
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: BraveWallet.NetworkInfo.mockSolana.nativeToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = BraveWallet.NetworkInfo.mockSolana.nativeToken
    let ex = expectation(description: "send-sol-transaction")
    store.sendToken(
      amount: "0.01"
    ) { success, errMsg in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }

    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test making Solana Token Program Transfer transaction
  func testSolSendTokenProgramTransfer() {
    let splTokenBalance = "1000000"
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      userAssets: [.mockSolana: [.mockSpdToken]],
      selectedCoin: .sol,
      splTokenBalance: splTokenBalance
    )
    solTxManagerProxy._makeTokenProgramTransferTxData = { _, _, _, _, _, _, completion in
      completion(.init(), .success, "")
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .mockSpdToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .mockSpdToken
    let ex = expectation(description: "send-sol-transaction")
    store.sendToken(
      amount: "0.01"
    ) { success, errMsg in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }

    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test making a FIL Send transaction on filecoin mainnet
  func testMakeSendFILTransaction() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .mockFilToken
    let ex = expectation(description: "send-fil-transaction")
    store.sendToken(amount: "1") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testMakeSendBTCTransaction() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .mockBTCToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .mockBTCToken
    let ex = expectation(description: "send-btc-transaction")
    store.sendToken(amount: "0.0005") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test Solana System Program transaction is created with correct lamports value for the `mockSolToken` (9 decimals)
  func testSendSolAmount() {
    let mockBalance: UInt64 = 47
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      userAssets: [.mockSolana: [.mockSolToken, .mockSpdToken]],
      selectedCoin: .sol,
      solanaBalance: mockBalance
    )
    solTxManagerProxy._makeSystemProgramTransferTxData = { _, _, lamports, completion in
      let solValueString = "10000000"  // 0.01
      XCTAssertNotNil(UInt64(solValueString))
      XCTAssertEqual(lamports, UInt64(solValueString)!)
      completion(.init(), .success, "")
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .mockSolToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .mockSolToken
    let ex = expectation(description: "send-sol-transaction")
    store.sendToken(
      amount: "0.01"
    ) { success, errMsg in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }

    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test Solana SPL Token Program transaction is created with correct amount value for the `mockSpdToken` (6 decimals)
  func testSendSplTokenAmount() {
    let mockBalance: UInt64 = 47
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      userAssets: [.mockSolana: [.mockSolToken, .mockSpdToken]],
      selectedCoin: .sol,
      solanaBalance: mockBalance
    )
    solTxManagerProxy._makeTokenProgramTransferTxData = { chainId, _, _, _, amount, _, completion in
      let splValueString = "10000"  // 0.01 SPD
      XCTAssertNotNil(UInt64(splValueString))
      XCTAssertEqual(amount, UInt64(splValueString)!)
      completion(.init(), .success, "")
    }

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .mockSpdToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .mockSpdToken
    let ex = expectation(description: "send-spl-transaction")
    store.sendToken(
      amount: "0.01"
    ) { success, errMsg in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }

    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testFetchSelectedSendNFTMetadataERC721() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()
    ethTxManagerProxy._makeErc721TransferFromData = { _, _, _, _, completion in
      completion(true, .init())
    }

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let selectedSendNFTMetadataERC721Exception = expectation(
      description: "sendTokenStore-selectedSendTokenERC721MetadataException"
    )
    XCTAssertNil(store.selectedSendNFTMetadata)  // Initial state
    store.$selectedSendNFTMetadata
      .dropFirst()
      .collect(1)
      .sink { metadata in
        defer { selectedSendNFTMetadataERC721Exception.fulfill() }
        guard let lastUpdatedMetadata = metadata.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedMetadata?.image, self.mockERC721Metadata.image)
        XCTAssertEqual(lastUpdatedMetadata?.name, self.mockERC721Metadata.name)
        XCTAssertEqual(lastUpdatedMetadata?.desc, self.mockERC721Metadata.desc)
      }.store(in: &cancellables)

    store.selectedSendToken = .mockERC721NFTToken

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  func testFetchSelectedSendNFTMetadataSolNFT() {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices()
    ethTxManagerProxy._makeErc721TransferFromData = { _, _, _, _, completion in
      completion(true, .init())
    }

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let selectedSendNFTMetadataSolNFTException = expectation(
      description: "sendTokenStore-selectedSendNFTMetadataSolNFTException"
    )
    XCTAssertNil(store.selectedSendNFTMetadata)  // Initial state
    store.$selectedSendNFTMetadata
      .dropFirst()
      .collect(1)
      .sink { metadata in
        defer { selectedSendNFTMetadataSolNFTException.fulfill() }
        guard let lastUpdatedMetadata = metadata.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedMetadata?.image, self.mockSolMetadata.image)
        XCTAssertEqual(lastUpdatedMetadata?.name, self.mockSolMetadata.name)
        XCTAssertEqual(lastUpdatedMetadata?.desc, self.mockSolMetadata.desc)
      }.store(in: &cancellables)

    store.selectedSendToken = .mockSolanaNFTToken

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `resolvedAddress` will be assigned the address returned from `snsGetSolAddr`.
  func testSNSAddressResolution() {
    let domain = "brave.sol"
    let expectedAddress = "xxxxxxxxxxyyyyyyyyyyzzzzzzzzzz0000000000"

    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedAccount: .mockSolAccount,
      selectedCoin: .sol,
      snsGetSolAddr: expectedAddress
    )

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let resolvedAddressExpectation = expectation(description: "sendTokenStore-resolvedAddress")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$resolvedAddress
      // Initial value, reset to nil in `sendAddress` didSet, reset to nil in `validateSolanaSendAddress`
      .dropFirst(3)
      .sink { resolvedAddress in
        defer { resolvedAddressExpectation.fulfill() }
        XCTAssertEqual(resolvedAddress, expectedAddress)
      }.store(in: &cancellables)

    store.sendAddress = domain

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `addressError` will be assigned an `ensError` if error is returned from `snsGetSolAddr`.
  func testSNSAddressResolutionFailure() {
    let domain = "brave.sol"
    let expectedAddress = ""
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedAccount: .mockSolAccount,
      selectedCoin: .sol,
      snsGetSolAddr: expectedAddress
    )

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let resolvedAddressExpectation = expectation(description: "sendTokenStore-resolvedAddress")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$resolvedAddress
      .dropFirst(2)  // Initial value, reset to nil in `validateSolanaSendAddress`
      .sink { resolvedAddress in
        defer { resolvedAddressExpectation.fulfill() }
        XCTAssertNil(resolvedAddress)
      }.store(in: &cancellables)

    let addressErrorExpectation = expectation(description: "sendTokenStore-addressError")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$addressError
      .dropFirst()  // Initial value
      .sink { addressError in
        defer { addressErrorExpectation.fulfill() }
        XCTAssertEqual(addressError, .snsError(domain: domain))
      }.store(in: &cancellables)

    store.sendAddress = domain

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `resolvedAddress` will be used in the ethereum transaction when available.
  func testResolvedAddressUsedInSolTxIfAvailable() {
    let domain = "brave.sol"
    let expectedAddress = "xxxxxxxxxxyyyyyyyyyyzzzzzzzzzz0000000000"
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedAccount: .mockSolAccount,
      selectedCoin: .sol,
      snsGetSolAddr: expectedAddress
    )
    var createdWithToAddress: String?
    solTxManagerProxy._makeSystemProgramTransferTxData = { _, toAddress, _, completion in
      createdWithToAddress = toAddress
      completion(.init(), .success, "")
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .mockSolToken

    let resolvedAddressExpectation = expectation(description: "sendTokenStore-resolvedAddress")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$resolvedAddress
      // Initial value, reset to nil in `sendAddress` didSet, reset to nil in `validateSolanaSendAddress`
      .dropFirst(3)
      .sink { resolvedAddress in
        defer { resolvedAddressExpectation.fulfill() }
        XCTAssertEqual(resolvedAddress, expectedAddress)
      }.store(in: &cancellables)
    store.sendAddress = domain
    // wait for resolved domain to populate
    wait(for: [resolvedAddressExpectation], timeout: 1)

    let ex = expectation(description: "send-transaction")
    store.sendToken(amount: "0.01") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
      XCTAssertNotNil(createdWithToAddress)
      XCTAssertEqual(createdWithToAddress, expectedAddress)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `resolvedAddress` will be assigned the address returned from `ensGetEthAddr`.
  func testENSAddressResolution() {
    let domain = "brave.eth"
    let expectedAddress = "0xxxxxxxxxxxyyyyyyyyyyzzzzzzzzzz0000000000"

    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedCoin: .eth,
      ensGetEthAddr: expectedAddress
    )

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let resolvedAddressExpectation = expectation(description: "sendTokenStore-resolvedAddress")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$resolvedAddress
      // Initial value, reset to nil in `sendAddress` didSet, reset to nil in `validateEthereumSendAddress`
      .dropFirst(3)
      .sink { resolvedAddress in
        defer { resolvedAddressExpectation.fulfill() }
        XCTAssertEqual(resolvedAddress, expectedAddress)
      }.store(in: &cancellables)

    store.sendAddress = domain

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `isOffchainResolveRequired` will be assigned `true` when `ensGetEthAddr` returns true for offchain consent required, and that `resolvedAddress` remains nil
  func testENSAddressResolutionOffchain() {
    let domain = "braveoffchain.eth"
    let expectedAddress = ""

    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedCoin: .eth,
      ensGetEthAddr: expectedAddress
    )
    rpcService._ensGetEthAddr = { _, completion in
      completion(expectedAddress, true, .success, "")
    }

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let isOffchainResolveRequiredExpectation = expectation(
      description: "sendTokenStore-isOffchainResolveRequired"
    )
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$isOffchainResolveRequired
      .dropFirst(2)  // Initial value, reset to false for `sendAddress` assignment
      .sink { isOffchainResolveRequired in
        defer { isOffchainResolveRequiredExpectation.fulfill() }
        XCTAssertTrue(isOffchainResolveRequired)
        XCTAssertNil(store.resolvedAddress)
      }.store(in: &cancellables)

    store.sendAddress = domain

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `addressError` will be assigned an `ensError` if error is returned from `ensGetEthAddr`.
  func testENSAddressResolutionFailure() {
    let domain = "brave.eth"
    let expectedAddress = ""
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedCoin: .eth,
      ensGetEthAddr: expectedAddress
    )

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let resolvedAddressExpectation = expectation(description: "sendTokenStore-resolvedAddress")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$resolvedAddress
      .dropFirst(2)  // Initial value, reset to nil in `validateEthereumSendAddress`
      .sink { resolvedAddress in
        defer { resolvedAddressExpectation.fulfill() }
        XCTAssertNil(resolvedAddress)
      }.store(in: &cancellables)

    let addressErrorExpectation = expectation(description: "sendTokenStore-addressError")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$addressError
      .dropFirst()  // Initial value
      .sink { addressError in
        defer { addressErrorExpectation.fulfill() }
        XCTAssertEqual(addressError, .ensError(domain: domain))
      }.store(in: &cancellables)

    store.sendAddress = domain

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `resolvedAddress` will be used in the ethereum transaction when available.
  func testResolvedAddressUsedInEthTxIfAvailable() {
    let domain = "brave.eth"
    let expectedAddress = "0xxxxxxxxxxxyyyyyyyyyyzzzzzzzzzz0000000000"
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedCoin: .eth,
      ensGetEthAddr: expectedAddress
    )
    var createdWithToAddress: String?
    ethTxManagerProxy._makeErc20TransferData = { toAddress, v2, completion in
      createdWithToAddress = toAddress
      completion(true, [])
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    store.selectedSendToken = .mockSolToken

    let resolvedAddressExpectation = expectation(description: "sendTokenStore-resolvedAddress")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$resolvedAddress
      // Initial value, reset to nil in `sendAddress` didSet, reset to nil in `validateEthereumSendAddress`
      .dropFirst(3)
      .sink { resolvedAddress in
        defer {
          resolvedAddressExpectation.fulfill()
        }
        XCTAssertEqual(resolvedAddress, expectedAddress)
      }.store(in: &cancellables)
    store.sendAddress = domain
    // wait for resolved domain to populate
    wait(for: [resolvedAddressExpectation], timeout: 1)

    let ex = expectation(description: "send-transaction")
    store.sendToken(amount: "0.01") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
      XCTAssertNotNil(createdWithToAddress)
      XCTAssertEqual(createdWithToAddress, expectedAddress)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `resolvedAddress` will be assigned the address returned from `unstoppableDomainsGetWalletAddr` when a Ethereum network is selected.
  func testUDAddressResolutionEthNetwork() {
    let domain = "brave.crypto"
    let expectedAddress = "0xxxxxxxxxxxyyyyyyyyyyzzzzzzzzzz0000000000"

    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedCoin: .eth,
      unstoppableDomainsGetWalletAddr: expectedAddress
    )

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let waitForPrefilledTokenExpectation = expectation(description: "waitForPrefilledToken")
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { waitForPrefilledTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, .previewToken)
      }
      .store(in: &cancellables)
    store.update()
    // wait for store to be setup with given `prefilledToken`
    wait(for: [waitForPrefilledTokenExpectation], timeout: 1)
    // release above sink on `selectedSendToken`
    // to avoid repeated calls to expectation
    cancellables.removeAll()

    let resolvedAddressExpectation = expectation(description: "sendTokenStore-resolvedAddress")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$resolvedAddress
      // Initial value, reset to nil in `sendAddress` didSet, reset to nil in `validateEthereumSendAddress`
      .dropFirst(3)
      .sink { resolvedAddress in
        defer { resolvedAddressExpectation.fulfill() }
        XCTAssertEqual(resolvedAddress, expectedAddress)
      }.store(in: &cancellables)

    store.sendAddress = domain

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `addressError` will be assigned an `ensError` if error is returned from `unstoppableDomainsGetWalletAddr`.
  func testUDAddressResolutionFailure() {
    let domain = "brave.eth"
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedCoin: .eth
    )
    rpcService._unstoppableDomainsGetWalletAddr = { _, _, completion in
      completion("", .internalError, "Something went wrong")
    }

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let waitForPrefilledTokenExpectation = expectation(description: "waitForPrefilledToken")
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { waitForPrefilledTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, .previewToken)
      }
      .store(in: &cancellables)
    store.update()
    // wait for store to be setup with given `prefilledToken`
    wait(for: [waitForPrefilledTokenExpectation], timeout: 1)
    // release above sink on `selectedSendToken`
    // to avoid repeated calls to expectation
    cancellables.removeAll()

    let resolvedAddressExpectation = expectation(description: "sendTokenStore-resolvedAddress")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$resolvedAddress
      .dropFirst(2)  // Initial value, reset to nil in `validateEthereumSendAddress`
      .sink { resolvedAddress in
        defer { resolvedAddressExpectation.fulfill() }
        XCTAssertNil(resolvedAddress)
      }.store(in: &cancellables)

    let addressErrorExpectation = expectation(description: "sendTokenStore-addressError")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$addressError
      .dropFirst()  // Initial value
      .sink { addressError in
        defer { addressErrorExpectation.fulfill() }
        XCTAssertEqual(addressError, .ensError(domain: domain))
      }.store(in: &cancellables)

    store.sendAddress = domain

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `resolvedAddress` will be assigned the address returned from `unstoppableDomainsGetWalletAddr` when a Solana network is selected.
  func testUDAddressResolutionSolNetwork() {
    let domain = "brave.crypto"
    let expectedAddress = "xxxxxxxxxxyyyyyyyyyyzzzzzzzzzz0000000000"

    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedCoin: .sol,
      unstoppableDomainsGetWalletAddr: expectedAddress
    )

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .mockSolToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let waitForPrefilledTokenExpectation = expectation(description: "waitForPrefilledToken")
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { waitForPrefilledTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, .mockSolToken)
      }
      .store(in: &cancellables)
    store.update()
    // wait for store to be setup with given `prefilledToken`
    wait(for: [waitForPrefilledTokenExpectation], timeout: 1)
    // release above sink on `selectedSendToken`
    // to avoid repeated calls to expectation
    cancellables.removeAll()

    let resolvedAddressExpectation = expectation(description: "sendTokenStore-resolvedAddress")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$resolvedAddress
      // Initial value, reset to nil in `sendAddress` didSet, reset to nil in `validateSolanaSendAddress`
      .dropFirst(3)
      .sink { resolvedAddress in
        defer { resolvedAddressExpectation.fulfill() }
        XCTAssertEqual(resolvedAddress, expectedAddress)
      }.store(in: &cancellables)

    store.sendAddress = domain

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `resolvedAddress` will be assigned the address returned from `unstoppableDomainsGetWalletAddr`, then if the `selectedSendToken` changes will call `unstoppableDomainsGetWalletAddr` and assign the new `resolvedAddress`.
  func testUDAddressResolutionTokenChange() {
    let domain = "brave.crypto"
    let expectedAddress = "0xxxxxxxxxxxyyyyyyyyyyzzzzzzzzzz0000000000"
    let expectedAddressUSDC = "0x1111111111222222222233333333330000000000"

    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedCoin: .eth
    )
    rpcService._unstoppableDomainsGetWalletAddr = { domain, token, completion in
      if token == .mockUSDCToken {  // simulate special address for USDC
        completion(expectedAddressUSDC, .success, "")
      } else {
        completion(expectedAddress, .success, "")
      }
    }

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    let waitForPrefilledTokenExpectation = expectation(description: "waitForPrefilledToken")
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { waitForPrefilledTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, .previewToken)
      }
      .store(in: &cancellables)
    store.update()
    // wait for store to be setup with given `prefilledToken`
    wait(for: [waitForPrefilledTokenExpectation], timeout: 1)
    // release above sink on `selectedSendToken`
    // to avoid repeated calls to expectation
    cancellables.removeAll()

    // Wait for `resolvedAddress` to be populated with address for the `prefilledToken`
    let resolvedAddressExpectation = expectation(description: "sendTokenStore-resolvedAddress")
    XCTAssertNil(store.resolvedAddress)  // Initial state
    store.$resolvedAddress
      // Initial value, reset to nil in `sendAddress` didSet, reset to nil in `resolveUnstoppableDomain`, assigned address in `resolveUnstoppableDomain`, reset to nil in `resolveUnstoppableDomain`, assigned address in `resolveUnstoppableDomain`
      .collect(6)
      .sink { resolvedAddresses in
        guard let resolvedAddress = resolvedAddresses.last else {
          XCTFail("Expected >0 resolved address assignments")
          return
        }
        XCTAssertEqual(store.selectedSendToken, .previewToken)
        XCTAssertEqual(resolvedAddress, expectedAddress)
        resolvedAddressExpectation.fulfill()
      }.store(in: &cancellables)

    store.sendAddress = domain
    wait(for: [resolvedAddressExpectation], timeout: 1)
    // release above sink on `selectedSendToken`
    // to avoid repeated calls to expectation
    cancellables.removeAll()

    // Verify change from the resolved address for `previewToken`
    // will change to the resolved address for `.mockUSDCToken`
    let resolvedUSDCAddressExpectation = expectation(
      description: "sendTokenStore-resolvedUSDCAddress"
    )
    store.$resolvedAddress
      .collect(3)
      .sink { resolvedAddresses in
        // initial value of sink
        XCTAssertEqual(resolvedAddresses[safe: 0], expectedAddress)
        // reset in `resolveUnstoppableDomain`
        XCTAssertEqual(resolvedAddresses[safe: 1], Optional<String>.none)
        // new value assigned after resolving for new token
        XCTAssertEqual(resolvedAddresses[safe: 2], expectedAddressUSDC)
        XCTAssertEqual(store.selectedSendToken, .mockUSDCToken)
        resolvedUSDCAddressExpectation.fulfill()
      }.store(in: &cancellables)

    // change token from `.previewToken` to `.mockUSDCToken`
    store.selectedSendToken = .mockUSDCToken

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  private let account: BraveWallet.AccountInfo = .previewAccount
  private let ethSepolia: BraveWallet.BlockchainToken = BraveWallet.NetworkInfo.mockSepolia
    .nativeToken
    .copy(asVisibleAsset: true)
  private let usdcSepolia: BraveWallet.BlockchainToken = .mockUSDCToken
    .copy(asVisibleAsset: true).then {
      $0.chainId = BraveWallet.SepoliaChainId
    }
  private let account2: BraveWallet.AccountInfo = .init(
    accountId: .init(
      coin: .eth,
      keyringId: BraveWallet.KeyringId.default,
      kind: .derived,
      address: "mock_eth_id_2",
      accountIndex: 0,
      uniqueKey: "mock_eth_id_2"
    ),
    address: "mock_eth_id_2",
    name: "Ethereum Account 2",
    hardware: nil
  )

  /// Test `didSelect(account:token:)` with a new token that is on the currently selected account and currently selected network.
  @MainActor func testDidSelectSameAccountSameNetwork() async {
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedAccount: account,
      userAssets: [.mockSepolia: [ethSepolia, usdcSepolia]],
      selectedCoin: .eth
    )
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: ethSepolia,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    let sendTokenExpectation = expectation(description: "sendTokenExpectation")
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { sendTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, self.ethSepolia)
      }
      .store(in: &cancellables)
    store.update()
    // wait for `prefilledToken` to be assigned.
    await fulfillment(of: [sendTokenExpectation], timeout: 1)
    cancellables.removeAll()

    let didSelectSendTokenExpectation = expectation(description: "didSelectSendTokenExpectation")
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { didSelectSendTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, self.usdcSepolia)
      }
      .store(in: &cancellables)
    store.didSelect(account: account, token: usdcSepolia)
    await fulfillment(of: [didSelectSendTokenExpectation], timeout: 1)
  }

  /// Test `didSelect(account:token:)` with a new token that is not on the currently selected account but is on the currently selected network.
  @MainActor func testDidSelectNewAccountSameNetwork() async {
    let account: BraveWallet.AccountInfo = .previewAccount
    let ethSepolia: BraveWallet.BlockchainToken = BraveWallet.NetworkInfo.mockSepolia.nativeToken
      .copy(asVisibleAsset: true)
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedAccount: account,
      userAssets: [.mockSepolia: [ethSepolia]],
      selectedCoin: .eth
    )
    let setSelectedAccountExpectation = expectation(description: "setSelectedAccountExpectation")
    keyringService._setSelectedAccount = { accountId, completion in
      defer { setSelectedAccountExpectation.fulfill() }
      XCTAssertEqual(accountId.address, self.account2.address)
      completion(true)
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: ethSepolia,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    let sendTokenExpectation = expectation(description: "sendTokenExpectation")
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { sendTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, ethSepolia)
      }
      .store(in: &cancellables)
    store.update()
    // wait for `prefilledToken` to be assigned.
    await fulfillment(of: [sendTokenExpectation], timeout: 1)
    cancellables.removeAll()

    store.didSelect(account: account2, token: ethSepolia)
    await fulfillment(of: [setSelectedAccountExpectation], timeout: 1)
  }

  /// Test `didSelect(account:token:)` with a new token that is not on the currently selected account and not on the currently selected network.
  @MainActor func testDidSelectNewAccountNewNetwork() async {
    let ethMainnet: BraveWallet.BlockchainToken = BraveWallet.NetworkInfo.mockMainnet.nativeToken
      .copy(asVisibleAsset: true)
    var selectedNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedAccount: account,
      userAssets: [.mockMainnet: [ethMainnet], .mockSepolia: [usdcSepolia]],
      selectedCoin: .eth
    )
    let setSelectedAccountExpectation = expectation(description: "setSelectedAccountExpectation")
    keyringService._setSelectedAccount = { accountId, completion in
      defer { setSelectedAccountExpectation.fulfill() }
      XCTAssertEqual(accountId.address, self.account2.address)
      completion(true)
    }
    let setNetworkExpectation = expectation(description: "setNetworkExpectation")
    rpcService._setNetwork = { chainId, coin, origin, completion in
      defer { setNetworkExpectation.fulfill() }
      XCTAssertEqual(chainId, self.usdcSepolia.chainId)
      selectedNetwork =
        MockJsonRpcService.allKnownNetworks.first(where: { $0.chainId == chainId })
        ?? selectedNetwork
      completion(true)
    }
    rpcService._network = { coin, origin, completion in
      completion(selectedNetwork)
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: ethMainnet,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    let sendTokenExpectation = expectation(description: "sendTokenExpectation")
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { sendTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, ethMainnet)
      }
      .store(in: &cancellables)
    store.update()
    // wait for `prefilledToken` to be assigned.
    await fulfillment(of: [sendTokenExpectation], timeout: 1)
    cancellables.removeAll()

    let didSelectSendTokenExpectation = expectation(description: "didSelectSendTokenExpectation")
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { didSelectSendTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, self.usdcSepolia)
      }
      .store(in: &cancellables)
    store.didSelect(account: account2, token: usdcSepolia)
    await fulfillment(
      of: [didSelectSendTokenExpectation, setSelectedAccountExpectation, setNetworkExpectation],
      timeout: 1
    )
  }

  /// Test `didSelect(account:token:)` with a new token that is on a different coin type (ex. Ethereum -> Solana, Solana -> Filecoin).
  @MainActor func testDidSelectCoinTypeSwitch() async {
    let solMainnet: BraveWallet.BlockchainToken = BraveWallet.NetworkInfo.mockSolana.nativeToken
      .copy(asVisibleAsset: true)
    let filTokenMainnet: BraveWallet.BlockchainToken = .mockFilToken.copy(asVisibleAsset: true)
    var selectedNetwork: BraveWallet.NetworkInfo = .mockSepolia
    var selectedNetworkToBe: BraveWallet.NetworkInfo = .mockSolana
    var selectedAccount: BraveWallet.AccountInfo = account
    let (
      keyringService, rpcService, walletService, ethTxManagerProxy, solTxManagerProxy,
      bitcoinWalletService, mockAssetManager
    ) = setupServices(
      selectedAccount: selectedAccount,
      userAssets: [
        .mockSepolia: [usdcSepolia], .mockSolana: [solMainnet],
        .mockFilecoinMainnet: [filTokenMainnet],
      ],
      selectedCoin: .eth
    )

    let didSelectSendFilTokenExpectation = expectation(
      description: "didSelectSendFilTokenExpectation"
    )
    let setSelectedFilAccountExpectation = expectation(
      description: "setSelectedFilAccountExpectation"
    )
    let setFilNetworkExpectation = expectation(description: "setFilNetworkExpectation")

    let didSelectSendTokenExpectation = expectation(description: "didSelectSendTokenExpectation")
    let setSelectedAccountExpectation = expectation(description: "setSelectedAccountExpectation")
    let setNetworkExpectation = expectation(description: "setNetworkExpectation")

    keyringService._setSelectedAccount = { accountId, completion in
      defer {
        if accountId.coin == .sol {
          setSelectedAccountExpectation.fulfill()
        } else {
          setSelectedFilAccountExpectation.fulfill()
        }
      }
      XCTAssertEqual(accountId.address, selectedAccount.address)
      completion(true)
    }
    rpcService._setNetwork = { chainId, coin, origin, completion in
      defer {
        if coin == .sol {
          setNetworkExpectation.fulfill()
        } else {
          setFilNetworkExpectation.fulfill()
        }
      }
      XCTAssertEqual(chainId, selectedNetworkToBe.chainId)
      selectedNetwork =
        MockJsonRpcService.allKnownNetworks.first(where: { $0.chainId == chainId })
        ?? selectedNetwork
      completion(true)
    }
    rpcService._network = { coin, origin, completion in
      completion(selectedNetwork)
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: usdcSepolia,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    let sendTokenExpectation = expectation(description: "sendTokenExpectation")
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer { sendTokenExpectation.fulfill() }
        XCTAssertEqual(selectedSendToken, self.usdcSepolia)
      }
      .store(in: &cancellables)
    store.update()
    // wait for `prefilledToken` to be assigned.
    await fulfillment(of: [sendTokenExpectation], timeout: 1)
    cancellables.removeAll()

    // select solana mainnet
    selectedAccount = .mockSolAccount
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer {
          didSelectSendTokenExpectation.fulfill()
        }
        XCTAssertEqual(selectedSendToken, solMainnet)
      }
      .store(in: &cancellables)
    store.didSelect(account: .mockSolAccount, token: solMainnet)
    await fulfillment(
      of: [didSelectSendTokenExpectation, setSelectedAccountExpectation, setNetworkExpectation],
      timeout: 1
    )
    cancellables.removeAll()

    // select filecoin mainnet
    selectedAccount = .mockFilAccount
    selectedNetworkToBe = .mockFilecoinMainnet
    store.$selectedSendToken
      .dropFirst()
      .sink { selectedSendToken in
        defer {
          didSelectSendFilTokenExpectation.fulfill()
        }
        XCTAssertEqual(selectedSendToken, filTokenMainnet)
      }
      .store(in: &cancellables)
    store.didSelect(account: .mockFilAccount, token: filTokenMainnet)
    await fulfillment(
      of: [
        didSelectSendFilTokenExpectation, setSelectedFilAccountExpectation,
        setFilNetworkExpectation,
      ],
      timeout: 1
    )
  }
}
