// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Combine
import BraveCore
import BigNumber
@testable import BraveWallet

class SendTokenStoreTests: XCTestCase {
  private var cancellables: Set<AnyCancellable> = []
  private let batSymbol = "BAT"
  
  private func setupServices(
    accountAddress: String = BraveWallet.AccountInfo.previewAccount.address,
    userAssets: [BraveWallet.BlockchainToken] = [.previewToken],
    selectedCoin: BraveWallet.CoinType = .eth,
    selectedNetwork: BraveWallet.NetworkInfo = .mockGoerli,
    balance: String = "0",
    erc20Balance: String = "0",
    erc721Balance: String = "0",
    solanaBalance: UInt64 = 0,
    splTokenBalance: String = "0"
  ) -> (BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService, BraveWallet.TestBraveWalletService, BraveWallet.TestSolanaTxManagerProxy) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._selectedAccount = { $1(accountAddress) }
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._network = { $1(selectedNetwork) }
    rpcService._allNetworks = { $1([selectedNetwork]) }
    rpcService._addObserver = { _ in }
    rpcService._balance = { $3(balance, .success, "") }
    rpcService._erc20TokenBalance = { $3(erc20Balance, .success, "") }
    rpcService._erc721TokenBalance = { $4(erc721Balance, .success, "") }
    rpcService._solanaBalance = { $2(solanaBalance, .success, "") }
    rpcService._splTokenAccountBalance = {_, _, _, completion in
      completion(splTokenBalance, UInt8(6), "", .success, "")
    }
    rpcService._erc721Metadata = { _, _, _, completion in
      completion(
      """
      {
        "image": "mock.image.url",
        "name": "mock nft name",
        "description": "mock nft description"
      }
      """, .success, "")
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._selectedCoin = { $0(selectedCoin) }
    walletService._userAssets = { $2(userAssets) }
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    return (keyringService, rpcService, walletService, solTxManagerProxy)
  }
  
  /// Test given a `prefilledToken` will be assigned to `selectedSendToken`
  func testPrefilledToken() {
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()

    var store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: nil
    )
    XCTAssertNil(store.selectedSendToken)

    store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .previewToken
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
    var selectedCoin: BraveWallet.CoinType = .eth
    var selectedNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let (keyringService, rpcService, walletService, solTxManagerProxy) = setupServices()
    walletService._selectedCoin = { $0(selectedCoin) }
    rpcService._network = { coin, completion in
      completion(selectedNetwork)
    }
    rpcService._allNetworks = { coin, completion in
      completion(coin == .eth ? [.mockMainnet] : [.mockSolana])
    }
    // simulate network switch when `setNetwork` is called
    rpcService._setNetwork = { chainId, coin, completion in
      XCTAssertEqual(chainId, BraveWallet.SolanaMainnet) // verify network switched to SolanaMainnet
      selectedCoin = coin
      selectedNetwork = coin == .eth ? .mockMainnet : .mockSolana
      completion(true)
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .mockSolToken
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
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: numDecimals))
    let mockBalanceWei = formatter.weiString(from: mockDecimalBalance, radix: .hex, decimals: numDecimals) ?? ""
    
    let (keyringService, rpcService, walletService, solTxManagerProxy) = setupServices(balance: mockBalanceWei)
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: nil
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
        XCTAssertEqual(selectedSendToken.symbol, BraveWallet.NetworkInfo.mockGoerli.nativeToken.symbol)
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
    let (keyringService, rpcService, walletService, solTxManagerProxy) = setupServices()
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .previewToken
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
    let (keyringService, rpcService, walletService, solTxManagerProxy) = setupServices()
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .previewToken
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
    let (keyringService, rpcService, walletService, solTxManagerProxy) = setupServices()
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: nil
    )
    let token: BraveWallet.BlockchainToken = .init(contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef", name: "Basic Attention Token", logo: "", isErc20: true, isErc721: false, isNft: false, symbol: batSymbol, decimals: 18, visible: true, tokenId: "", coingeckoId: "", chainId: "", coin: .eth)
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
    let (keyringService, rpcService, walletService, solTxManagerProxy) = setupServices()
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .previewToken
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
    let (keyringService, rpcService, walletService, solTxManagerProxy) = setupServices()
    let ethTxManagerProxy = BraveWallet.TestEthTxManagerProxy()
    ethTxManagerProxy._makeErc721TransferFromData = { _, _, _, _, completion in
      completion(true, .init())
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .mockERC721NFTToken
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
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    let mockBalance = "47.156499657504857477"
    let mockBalanceWei = formatter.weiString(from: mockBalance, radix: .hex, decimals: 18) ?? ""

    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._chainId = { $1(BraveWallet.NetworkInfo.mockGoerli.chainId) }
    rpcService._network = { $1(BraveWallet.NetworkInfo.mockGoerli)}
    rpcService._allNetworks = { $1([.mockGoerli]) }
    rpcService._balance = { _, _, _, completion in
      completion(mockBalanceWei, .success, "")
    }
    rpcService._addObserver = { _ in }
    rpcService._erc721Metadata = { _, _, _, completion in
      completion("", .internalError, "")
    }

    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { $2([.previewToken]) }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.eth) }

    let keyringService = BraveWallet.TestKeyringService()
    keyringService._selectedAccount = { $1("account-address") }
    keyringService._addObserver = { _ in }

    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()

    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .previewToken
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

    store.update() // fetch balance
    waitForExpectations(timeout: 3) { error in // wait for balance to be updated
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
    store.suggestedAmountTapped(.all) // balance fetched, simulate user tapping 100%
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test making Solana System Program Transfer transaction
  func testSolSendSystemProgramTransfer() {
    let mockBalance: UInt64 = 47
    let (keyringService, rpcService, walletService, _) = setupServices(
      userAssets: [.mockSolToken],
      selectedCoin: .sol,
      selectedNetwork: .mockSolana,
      solanaBalance: mockBalance
    )
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeSystemProgramTransferTxData = {_, _, _, completion in
      completion(.init(), .success, "")
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: BraveWallet.NetworkInfo.mockSolana.nativeToken
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
    let (keyringService, rpcService, walletService, _) = setupServices(
      userAssets: [.mockSpdToken],
      selectedCoin: .sol,
      selectedNetwork: .mockSolana,
      splTokenBalance: splTokenBalance
    )
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeTokenProgramTransferTxData = { _, _, _, _, completion in
      completion(.init(), .success, "")
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .mockSpdToken
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
  
  /// Test Solana System Program transaction is created with correct lamports value for the `mockSolToken` (9 decimals)
  func testSendSolAmount() {
    let mockBalance: UInt64 = 47
    let (keyringService, rpcService, walletService, _) = setupServices(
      userAssets: [.mockSolToken, .mockSpdToken],
      selectedCoin: .sol,
      selectedNetwork: .mockSolana,
      solanaBalance: mockBalance
    )
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeSystemProgramTransferTxData = {_, _, lamports, completion in
      let solValueString = "10000000" // 0.01
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
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .mockSolToken
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
    let (keyringService, rpcService, walletService, _) = setupServices(
      userAssets: [.mockSolToken, .mockSpdToken],
      selectedCoin: .sol,
      selectedNetwork: .mockSolana,
      solanaBalance: mockBalance
    )
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeTokenProgramTransferTxData = { _, _, _, amount, completion in
      let splValueString = "10000" // 0.01 SPD
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
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .mockSpdToken
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

  func testFetchSelectedERC721Metadata() {
    let (keyringService, rpcService, walletService, solTxManagerProxy) = setupServices()
    let ethTxManagerProxy = BraveWallet.TestEthTxManagerProxy()
    ethTxManagerProxy._makeErc721TransferFromData = { _, _, _, _, completion in
      completion(true, .init())
    }
    let mockERC721Metadata: ERC721Metadata = .init(imageURLString: "mock.image.url", name: "mock nft name", description: "mock nft description")
    
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: nil
    )
    
    let selectedSendTokenERC721MetadataException = expectation(description: "accountActivityStore-selectedSendTokenERC721MetadataException")
    XCTAssertNil(store.selectedSendTokenERC721Metadata)  // Initial state
    store.$selectedSendTokenERC721Metadata
      .dropFirst()
      .collect(1)
      .sink { metadata in
        defer { selectedSendTokenERC721MetadataException.fulfill() }
        guard let lastUpdatedMetadata = metadata.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedMetadata?.imageURLString, mockERC721Metadata.imageURLString)
        XCTAssertEqual(lastUpdatedMetadata?.name, mockERC721Metadata.name)
        XCTAssertEqual(lastUpdatedMetadata?.description, mockERC721Metadata.description)
      }.store(in: &cancellables)
    
    store.selectedSendToken = .mockERC721NFTToken
    
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
}
