// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CustomDump
import XCTest

@testable import BraveWallet

extension BraveWallet.AccountInfo {
  fileprivate convenience init(
    address: String,
    name: String,
    coin: BraveWallet.CoinType = .eth
  ) {
    self.init(
      accountId: .init(
        coin: coin,
        keyringId: coin.keyringIds.first ?? .default,
        kind: .derived,
        address: address,
        accountIndex: 0,
        uniqueKey: address
      ),
      address: address,
      name: name,
      hardware: nil
    )
  }
}

class TransactionParserTests: XCTestCase {

  private let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  private let accountInfos: [BraveWallet.AccountInfo] = [
    BraveWallet.AccountInfo.previewAccount,
    (BraveWallet.AccountInfo.previewAccount.copy() as! BraveWallet.AccountInfo).then {
      $0.name = "Ethereum Account 2"
      $0.address = "0x0987654321098765432109876543210987654321"
      $0.accountId.uniqueKey = $0.address
      $0.accountId.address = "0x0987654321098765432109876543210987654321"
    },
    (BraveWallet.AccountInfo.mockSolAccount.copy() as! BraveWallet.AccountInfo).then {
      $0.name = "Solana Account 1"
      $0.address = "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd"
      $0.accountId.uniqueKey = $0.address
      $0.accountId.address = "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd"
    },
    (BraveWallet.AccountInfo.mockSolAccount.copy() as! BraveWallet.AccountInfo).then {
      $0.name = "Solana Account 2"
      $0.address = "0xeeeeeeeeeeffffffffff11111111112222222222"
      $0.accountId.uniqueKey = $0.address
      $0.accountId.address = "0xeeeeeeeeeeffffffffff11111111112222222222"
    },
    (BraveWallet.AccountInfo.mockFilTestnetAccount.copy() as! BraveWallet.AccountInfo).then {
      $0.name = "Filecoin Testnet 1"
      $0.address = "fil_testnet_address_1"
      $0.accountId.uniqueKey = $0.address
      $0.accountId.address = "fil_testnet_address_1"
    },
    (BraveWallet.AccountInfo.mockFilTestnetAccount.copy() as! BraveWallet.AccountInfo).then {
      $0.name = "Filecoin Testnet 2"
      $0.address = "fil_testnet_address_2"
      $0.accountId.uniqueKey = $0.address
      $0.accountId.address = "fil_testnet_address_2"
    },
    BraveWallet.AccountInfo.mockBtcAccount,
  ]
  private let tokens: [BraveWallet.BlockchainToken] = [
    .previewToken, .previewDaiToken, .mockUSDCToken, .mockSolToken, .mockSpdToken,
    .mockSolanaNFTToken, .mockFilToken, .mockBTCToken,
  ]
  let assetRatios: [String: Double] = [
    "eth": 1,
    BraveWallet.BlockchainToken.previewDaiToken.assetRatioId.lowercased(): 2,
    BraveWallet.BlockchainToken.mockUSDCToken.assetRatioId.lowercased(): 3,
    "sol": 20,
    BraveWallet.BlockchainToken.mockSpdToken.assetRatioId.lowercased(): 15,
    "fil": 2,
    BraveWallet.BlockchainToken.mockBTCToken.assetRatioId.lowercased(): 62_117,
  ]

  let mockGasEstimation = BraveWallet.GasEstimation1559(
    slowMaxPriorityFeePerGas: "0x0",
    slowMaxFeePerGas: "0x9",
    avgMaxPriorityFeePerGas: "0x59672ead",
    avgMaxFeePerGas: "0x59672eb6",
    fastMaxPriorityFeePerGas: "0x59682f00",
    fastMaxFeePerGas: "0x59682f09",
    baseFeePerGas: "0x9"
  )
  let mockSwapGasEstimation = BraveWallet.GasEstimation1559(
    slowMaxPriorityFeePerGas: "0x4ed3152b",
    slowMaxFeePerGas: "0x4ed31534",
    avgMaxPriorityFeePerGas: "0x59672ead",
    avgMaxFeePerGas: "0x59672eb6",
    fastMaxPriorityFeePerGas: "0x59682f00",
    fastMaxFeePerGas: "0x59682f09",
    baseFeePerGas: "0x9"
  )

  private func mockTransaction(
    fromAccount: BraveWallet.AccountInfo,
    txDataUnion: BraveWallet.TxDataUnion,
    txType: BraveWallet.TransactionType,
    txArgs: [String] = [],
    chainId: String = BraveWallet.MainnetChainId,
    effectiveRecipient: String,
    swapInfo: BraveWallet.SwapInfo? = nil
  ) -> BraveWallet.TransactionInfo {
    BraveWallet.TransactionInfo(
      id: "1",
      fromAddress: fromAccount.address,
      from: fromAccount.accountId,
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: txDataUnion,
      txStatus: .unapproved,
      txType: txType,
      txParams: [],
      txArgs: txArgs,
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil,
      chainId: chainId,
      effectiveRecipient: effectiveRecipient,
      isRetriable: false,
      swapInfo: swapInfo
    )
  }

  func testEthSendTransaction() {
    let network: BraveWallet.NetworkInfo = .mockMainnet

    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0x5208",
        to: "0x0987654321098765432109876543210987654321",
        value: "0x1b667a56d488000",  // 0.1234
        data: [],
        signOnly: false,
        signedTransaction: nil
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59672ead",
      maxFeePerGas: "0x59672eb6",
      gasEstimation: mockGasEstimation
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[0],
      txDataUnion: .init(ethTxData1559: transactionData),
      txType: .ethSend,
      txArgs: [],
      effectiveRecipient: transactionData.baseData.to
    )

    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAccountInfo: accountInfos[0],
      namedToAddress: "Ethereum Account 2",
      toAddress: "0x0987654321098765432109876543210987654321",
      network: .mockMainnet,
      details: .ethSend(
        .init(
          fromToken: network.nativeToken,
          fromValue: "0x1b667a56d488000",
          fromAmount: "0.1234",
          fromFiat: "$0.123",
          fromTokenMetadata: nil,
          gasFee: .init(
            fee: "0.0000314986",
            fiat: "$0.0000315"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [network, .mockPolygon],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: nil,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse ethSend transaction")
      return
    }
    XCTAssertEqual(
      expectedParsedTransaction.fromAccountInfo.id,
      parsedTransaction.fromAccountInfo.id
    )
    XCTAssertEqual(expectedParsedTransaction.namedFromAddress, parsedTransaction.namedFromAddress)
    XCTAssertEqual(expectedParsedTransaction.toAddress, parsedTransaction.toAddress)
    XCTAssertEqual(expectedParsedTransaction.networkSymbol, parsedTransaction.networkSymbol)
    guard case .ethSend(let expectedDetails) = expectedParsedTransaction.details,
      case .ethSend(let parsedDetails) = parsedTransaction.details
    else {
      XCTFail("Incorrectly parsed ethSend transaction")
      return
    }
    // `fromToken` to fail equatability check because `network.nativeToken` will because is a computed property
    XCTAssertEqual(expectedDetails.fromValue, parsedDetails.fromValue)
    XCTAssertEqual(expectedDetails.fromAmount, parsedDetails.fromAmount)
    XCTAssertEqual(expectedDetails.fromFiat, parsedDetails.fromFiat)
    XCTAssertEqual(expectedDetails.gasFee, parsedDetails.gasFee)
  }

  func testEthErc20TransferTransaction() {
    let network: BraveWallet.NetworkInfo = .mockMainnet

    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0xca48",
        to: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
        value: "0x0",
        data: [],
        signOnly: false,
        signedTransaction: nil
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59672ead",
      maxFeePerGas: "0x59672eb6",
      gasEstimation: mockGasEstimation
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[0],
      txDataUnion: .init(ethTxData1559: transactionData),
      txType: .erc20Transfer,
      txArgs: ["0x0987654321098765432109876543210987654321", "0x5ff20a91f724000"],
      effectiveRecipient: transactionData.baseData.to
    )

    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAccountInfo: accountInfos[0],
      namedToAddress: "Ethereum Account 2",
      toAddress: "0x0987654321098765432109876543210987654321",
      network: network,
      details: .erc20Transfer(
        .init(
          fromToken: .previewDaiToken,
          fromValue: "0x5ff20a91f724000",
          fromAmount: "0.4321",
          fromFiat: "$0.864",
          fromTokenMetadata: nil,
          gasFee: .init(
            fee: "0.0000776726",
            fiat: "$0.0000777"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [network, .mockPolygon],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: nil,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse erc20Transfer transaction")
      return
    }
    XCTAssertNoDifference(expectedParsedTransaction, parsedTransaction)
  }

  func testEthSwapTransaction() {
    let network: BraveWallet.NetworkInfo = .mockMainnet

    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0x4be75",
        to: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF",  // 0x exchange address
        value: "0x1b6951ef585a000",
        data: [],
        signOnly: false,
        signedTransaction: nil
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59682f00",
      maxFeePerGas: "0x59682f09",
      gasEstimation: mockSwapGasEstimation
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[0],
      txDataUnion: .init(ethTxData1559: transactionData),
      txType: .ethSwap,
      txArgs: [],
      effectiveRecipient: transactionData.baseData.to,
      swapInfo: .init(
        from: .eth,
        fromChainId: BraveWallet.MainnetChainId,
        fromAsset: BraveWallet.ethSwapAddress,
        fromAmount: "0x1b6951ef585a000",  // 0.12345 eth
        to: .eth,
        toChainId: BraveWallet.MainnetChainId,
        toAsset: "0xad6d458402f60fd3bd25163575031acdce07538d",
        toAmount: "0x5c6f2d76e910358b",  // 6.660592362643797387 dai
        receiver: "0x099140a37d5e1da04ce05294594d27a90a4cbc06",
        provider: "zeroex"
      )
    )

    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAccountInfo: accountInfos[0],
      namedToAddress: "0x Exchange Proxy",
      toAddress: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF",
      network: network,
      details: .ethSwap(
        .init(
          fromToken: .previewToken,
          fromNetwork: network,
          fromValue: "0x1b6951ef585a000",
          fromAmount: "0.12345",
          fromFiat: "$0.123",
          toToken: .previewDaiToken,
          toNetwork: network,
          minBuyValue: "0x5c6f2d76e910358b",
          minBuyAmount: "6.660592362643797387",
          minBuyAmountFiat: "$13.32",
          gasFee: .init(
            fee: "0.0004663515",
            fiat: "$0.000466"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [network, .mockPolygon],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: nil,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse ethSwap transaction")
      return
    }
    XCTAssertNoDifference(expectedParsedTransaction, parsedTransaction)
  }

  func testEthSwapTransactionUSDCToDAI() {
    let network: BraveWallet.NetworkInfo = .mockMainnet

    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0x4be75",
        to: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF",  // 0x exchange address
        value: "0x0",
        data: [],
        signOnly: false,
        signedTransaction: nil
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59682f00",
      maxFeePerGas: "0x59682f09",
      gasEstimation: mockSwapGasEstimation
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[0],
      txDataUnion: .init(ethTxData1559: transactionData),
      txType: .ethSwap,
      txArgs: [],
      effectiveRecipient: transactionData.baseData.to,
      swapInfo: .init(
        from: .eth,
        fromChainId: BraveWallet.MainnetChainId,
        fromAsset: "0x07865c6e87b9f70255377e024ace6630c1eaa37f",
        fromAmount: "0x16e360",  // 1.5 USDC
        to: .eth,
        toChainId: BraveWallet.MainnetChainId,
        toAsset: "0xad6d458402f60fd3bd25163575031acdce07538d",
        toAmount: "0x1bd02ca9a7c244e",  // ~0.1253 DAI
        receiver: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF",
        provider: "zeroex"
      )
    )

    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAccountInfo: accountInfos[0],
      namedToAddress: "0x Exchange Proxy",
      toAddress: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF",
      network: network,
      details: .ethSwap(
        .init(
          fromToken: .mockUSDCToken,
          fromNetwork: network,
          fromValue: "0x16e360",
          fromAmount: "1.5",
          fromFiat: "$4.50",
          toToken: .previewDaiToken,
          toNetwork: network,
          minBuyValue: "0x1bd02ca9a7c244e",
          minBuyAmount: "0.125259433834718286",
          minBuyAmountFiat: "$0.251",
          gasFee: .init(
            fee: "0.0004663515",
            fiat: "$0.000466"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [network, .mockPolygon],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: nil,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse ethSwap transaction")
      return
    }
    XCTAssertNoDifference(expectedParsedTransaction, parsedTransaction)
  }

  func testErc20ApproveTransaction() {
    let network: BraveWallet.NetworkInfo = .mockMainnet

    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0xb53f",
        to: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
        value: "0x0",
        data: [],
        signOnly: false,
        signedTransaction: nil
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59682f00",
      maxFeePerGas: "0x59682f09",
      gasEstimation: mockGasEstimation
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[0],
      txDataUnion: .init(ethTxData1559: transactionData),
      txType: .erc20Approve,
      txArgs: ["", "0x2386f26fc10000"],  // 0.01
      effectiveRecipient: transactionData.baseData.to
    )

    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAccountInfo: accountInfos[0],
      namedToAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress.truncatedAddress,
      toAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
      network: network,
      details: .ethErc20Approve(
        .init(
          token: .previewDaiToken,
          tokenContractAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
          approvalValue: "0x2386f26fc10000",
          approvalAmount: "0.01",
          approvalFiat: "$0.02",
          isUnlimited: false,
          spenderAddress: "",
          gasFee: .init(
            fee: "0.0000695985",
            fiat: "$0.0000696"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [network, .mockPolygon],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: nil,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse erc20Approve transaction")
      return
    }
    XCTAssertNoDifference(expectedParsedTransaction, parsedTransaction)
  }

  func testErc20ApproveTransactionUnlimited() {
    let network: BraveWallet.NetworkInfo = .mockMainnet

    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0xb53f",
        to: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
        value: "0x0",
        data: [],
        signOnly: false,
        signedTransaction: nil
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59682f00",
      maxFeePerGas: "0x59682f09",
      gasEstimation: mockGasEstimation
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[0],
      txDataUnion: .init(ethTxData1559: transactionData),
      txType: .erc20Approve,
      // unlimited
      txArgs: ["", "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"],
      effectiveRecipient: transactionData.baseData.to
    )

    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAccountInfo: accountInfos[0],
      namedToAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress.truncatedAddress,
      toAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
      network: network,
      details: .ethErc20Approve(
        .init(
          token: .previewDaiToken,
          tokenContractAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
          approvalValue: "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
          approvalAmount: "Unlimited",
          approvalFiat: "Unlimited",
          isUnlimited: true,
          spenderAddress: "",
          gasFee: .init(
            fee: "0.0000695985",
            fiat: "$0.0000696"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [network, .mockPolygon],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: nil,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse erc20Approve transaction")
      return
    }
    XCTAssertNoDifference(expectedParsedTransaction, parsedTransaction)
  }

  func testErc721TransferFromTransaction() {
    let network: BraveWallet.NetworkInfo = .mockMainnet

    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0x5208",
        to: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
        value: "0xde0b6b3a7640000",  // 1
        data: [],
        signOnly: false,
        signedTransaction: nil
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59672ead",
      maxFeePerGas: "0x59672eb6",
      gasEstimation: mockGasEstimation
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[0],
      txDataUnion: .init(ethTxData1559: transactionData),
      txType: .erc721TransferFrom,
      txArgs: [
        "0x1111111111aaaaaaaaaa2222222222bbbbbbbbbb",  // owner
        "0x0987654321098765432109876543210987654321",  // toAddress
        "token.id",
      ],
      effectiveRecipient: transactionData.baseData.to
    )

    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAccountInfo: accountInfos[0],
      namedToAddress: "Ethereum Account 2",
      toAddress: "0x0987654321098765432109876543210987654321",
      network: network,
      details: .erc721Transfer(
        .init(
          fromToken: .previewDaiToken,
          fromValue: "1",
          fromAmount: "1",
          nftMetadata: nil,
          owner: "0x1111111111aaaaaaaaaa2222222222bbbbbbbbbb",
          tokenId: "token.id",
          gasFee: .init(
            fee: "0.0000314986",
            fiat: "$0.0000315"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [network, .mockPolygon],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: nil,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse erc721TransferFrom transaction")
      return
    }
    XCTAssertNoDifference(expectedParsedTransaction, parsedTransaction)
  }

  func testSolanaSystemTransfer() {
    let network: BraveWallet.NetworkInfo = .mockSolana

    let transactionData: BraveWallet.SolanaTxData = .init(
      recentBlockhash: "",
      lastValidBlockHeight: 0,
      feePayer: accountInfos[2].address,
      toWalletAddress: accountInfos[3].address,
      tokenAddress: "",
      lamports: 100_000_000,
      amount: 0,
      txType: .solanaSystemTransfer,
      instructions: [
        .init(
          programId: "",
          accountMetas: [
            .init(pubkey: "", addrTableLookupIndex: nil, isSigner: false, isWritable: false)
          ],
          data: [],
          decodedData: nil
        )
      ],
      version: .legacy,
      messageHeader: .init(),
      staticAccountKeys: [],
      addressTableLookups: [],
      send: .init(maxRetries: .init(maxRetries: 1), preflightCommitment: nil, skipPreflight: nil),
      signTransactionParam: nil,
      feeEstimation: nil
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[2],
      txDataUnion: .init(solanaTxData: transactionData),
      txType: .solanaSystemTransfer,
      txArgs: [],
      chainId: network.chainId,
      effectiveRecipient: transactionData.toWalletAddress
    )
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[2].name,
      fromAccountInfo: accountInfos[2],
      namedToAddress: accountInfos[3].name,
      toAddress: accountInfos[3].address,
      network: network,
      details: .solSystemTransfer(
        .init(
          fromToken: .mockSolToken,
          fromValue: "100000000",
          fromAmount: "0.1",
          fromFiat: "$2.00",
          fromTokenMetadata: nil,
          gasFee: .init(
            fee: "0.00123",
            fiat: "$0.0246"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [.mockMainnet, .mockPolygon, network],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: 1_230_000,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse solanaSystemTransfer transaction")
      return
    }

    XCTAssertEqual(
      expectedParsedTransaction.fromAccountInfo.id,
      parsedTransaction.fromAccountInfo.id
    )
    XCTAssertEqual(expectedParsedTransaction.namedFromAddress, parsedTransaction.namedFromAddress)
    XCTAssertEqual(expectedParsedTransaction.toAddress, parsedTransaction.toAddress)
    XCTAssertEqual(expectedParsedTransaction.networkSymbol, parsedTransaction.networkSymbol)
    guard case .solSystemTransfer(let expectedDetails) = expectedParsedTransaction.details,
      case .solSystemTransfer(let parsedDetails) = parsedTransaction.details
    else {
      XCTFail("Incorrectly parsed solanaSystemTransfer transaction")
      return
    }
    // `fromToken` to fail equatability check because `network.nativeToken` will because is a computed property
    XCTAssertEqual(expectedDetails.fromValue, parsedDetails.fromValue)
    XCTAssertEqual(expectedDetails.fromAmount, parsedDetails.fromAmount)
    XCTAssertEqual(expectedDetails.fromFiat, parsedDetails.fromFiat)
    XCTAssertEqual(expectedDetails.gasFee, parsedDetails.gasFee)
  }

  func testSolanaSplTokenTransfer() {
    let network: BraveWallet.NetworkInfo = .mockSolana

    let transactionData: BraveWallet.SolanaTxData = .init(
      recentBlockhash: "",
      lastValidBlockHeight: 0,
      feePayer: accountInfos[2].address,
      toWalletAddress: accountInfos[3].address,
      tokenAddress: BraveWallet.BlockchainToken.mockSpdToken.contractAddress,
      lamports: 0,
      amount: 43_210_000,
      txType: .solanaSplTokenTransfer,
      instructions: [
        .init(
          programId: "",
          accountMetas: [
            .init(pubkey: "", addrTableLookupIndex: nil, isSigner: false, isWritable: false)
          ],
          data: [],
          decodedData: nil
        )
      ],
      version: .legacy,
      messageHeader: .init(),
      staticAccountKeys: [],
      addressTableLookups: [],
      send: .init(maxRetries: .init(maxRetries: 1), preflightCommitment: nil, skipPreflight: nil),
      signTransactionParam: nil,
      feeEstimation: nil
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[2],
      txDataUnion: .init(solanaTxData: transactionData),
      txType: .solanaSplTokenTransfer,
      txArgs: [],
      chainId: network.chainId,
      effectiveRecipient: transactionData.toWalletAddress
    )
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[2].name,
      fromAccountInfo: accountInfos[2],
      namedToAddress: accountInfos[3].name,
      toAddress: accountInfos[3].address,
      network: network,
      details: .solSplTokenTransfer(
        .init(
          fromToken: .mockSpdToken,
          fromValue: "43210000",
          fromAmount: "43.21",
          fromFiat: "$648.15",
          fromTokenMetadata: nil,
          gasFee: .init(
            fee: "0.0123",
            fiat: "$0.246"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [.mockMainnet, .mockPolygon, network],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: 12_300_000,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse solanaSplTokenTransfer transaction")
      return
    }

    XCTAssertEqual(expectedParsedTransaction, parsedTransaction)
  }

  func testSolanaNFTSplTokenTransfer() {
    let network: BraveWallet.NetworkInfo = .mockSolana

    let transactionData: BraveWallet.SolanaTxData = .init(
      recentBlockhash: "",
      lastValidBlockHeight: 0,
      feePayer: accountInfos[2].address,
      toWalletAddress: accountInfos[3].address,
      tokenAddress: BraveWallet.BlockchainToken.mockSolanaNFTToken.contractAddress,
      lamports: 0,
      amount: 1,
      txType: .solanaSplTokenTransfer,
      instructions: [
        .init(
          programId: "",
          accountMetas: [
            .init(pubkey: "", addrTableLookupIndex: nil, isSigner: false, isWritable: false)
          ],
          data: [],
          decodedData: nil
        )
      ],
      version: .legacy,
      messageHeader: .init(),
      staticAccountKeys: [],
      addressTableLookups: [],
      send: .init(maxRetries: .init(maxRetries: 1), preflightCommitment: nil, skipPreflight: nil),
      signTransactionParam: nil,
      feeEstimation: nil
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[2],
      txDataUnion: .init(solanaTxData: transactionData),
      txType: .solanaSplTokenTransfer,
      txArgs: [],
      chainId: network.chainId,
      effectiveRecipient: transactionData.toWalletAddress
    )
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[2].name,
      fromAccountInfo: accountInfos[2],
      namedToAddress: accountInfos[3].name,
      toAddress: accountInfos[3].address,
      network: network,
      details: .solSplTokenTransfer(
        .init(
          fromToken: .mockSolanaNFTToken,
          fromValue: "1",
          fromAmount: "1",
          fromFiat: "",
          fromTokenMetadata: nil,
          gasFee: .init(
            fee: "0.0123",
            fiat: "$0.246"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [.mockMainnet, .mockPolygon, network],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: 12_300_000,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse solanaSplTokenTransfer transaction")
      return
    }

    XCTAssertEqual(expectedParsedTransaction, parsedTransaction)
  }

  func testParseSolanaInstruction() {
    let fromPubkey = "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeee"
    let toPubkey = "zzzzzzzzzzyyyyyyyyyyxxxxxxxxxxwwwwwwwwwwvvvv"
    let transferInstruction = BraveWallet.SolanaInstruction(
      programId: BraveWallet.SolanaSystemProgramId,
      accountMetas: [
        .init(pubkey: fromPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
        .init(pubkey: toPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
      ],
      data: [],
      decodedData: .init(
        instructionType: UInt32(BraveWallet.SolanaSystemInstruction.transfer.rawValue),
        accountParams: [
          .init(name: BraveWallet.FromAccount, localizedName: "From Account"),
          .init(name: BraveWallet.ToAccount, localizedName: "To Account"),
        ],
        params: [
          .init(
            name: BraveWallet.Lamports,
            localizedName: "Lamports",
            value: "10000",
            type: .uint64
          )
        ]
      )
    )
    let expectedParsedTransfer = SolanaTxDetails.ParsedSolanaInstruction(
      name: "System Program - Transfer",
      details: [
        .init(key: "From Account", value: fromPubkey),
        .init(key: "To Account", value: toPubkey),
        .init(key: "Amount", value: "0.00001 SOL"),
        .init(key: "Lamports", value: "10000"),
      ],
      instruction: transferInstruction
    )
    XCTAssertNoDifference(
      expectedParsedTransfer,
      TransactionParser.parseSolanaInstruction(transferInstruction)
    )

    let withdrawNonceAccountInstruction = BraveWallet.SolanaInstruction(
      programId: BraveWallet.SolanaSystemProgramId,
      accountMetas: [
        .init(pubkey: fromPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
        .init(pubkey: toPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
        .init(
          pubkey: "SysvarRecentB1ockHashes11111111111111111111",
          addrTableLookupIndex: nil,
          isSigner: false,
          isWritable: false
        ),
        .init(
          pubkey: "SysvarRent111111111111111111111111111111111",
          addrTableLookupIndex: nil,
          isSigner: false,
          isWritable: false
        ),
        .init(pubkey: toPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
      ],
      data: [],
      decodedData: .init(
        instructionType: UInt32(BraveWallet.SolanaSystemInstruction.withdrawNonceAccount.rawValue),
        accountParams: [
          .init(name: BraveWallet.NonceAccount, localizedName: "Nonce Account"),
          .init(name: BraveWallet.ToAccount, localizedName: "To Account"),
          .init(name: "recentblockhashes_sysvar", localizedName: "RecentBlockhashes sysvar"),
          .init(name: "rent_sysvar", localizedName: "Rent sysvar"),
          .init(name: "nonce_authority", localizedName: "Nonce Authority"),
        ],
        params: [
          .init(name: BraveWallet.Lamports, localizedName: "Lamports", value: "40", type: .uint64)
        ]
      )
    )
    let expectedParsedWithdrawNonceAccount = SolanaTxDetails.ParsedSolanaInstruction(
      name: "System Program - Withdraw From Nonce Account",
      details: [
        .init(key: "Nonce Account", value: fromPubkey),
        .init(key: "To Account", value: toPubkey),
        .init(
          key: "RecentBlockhashes sysvar",
          value: "SysvarRecentB1ockHashes11111111111111111111"
        ),
        .init(key: "Rent sysvar", value: "SysvarRent111111111111111111111111111111111"),
        .init(key: "Nonce Authority", value: toPubkey),
        .init(key: "Amount", value: "0.00000004 SOL"),
        .init(key: "Lamports", value: "40"),
      ],
      instruction: withdrawNonceAccountInstruction
    )
    XCTAssertNoDifference(
      expectedParsedWithdrawNonceAccount,
      TransactionParser.parseSolanaInstruction(withdrawNonceAccountInstruction)
    )

    let createAccountInstruction = BraveWallet.SolanaInstruction(
      programId: BraveWallet.SolanaSystemProgramId,
      accountMetas: [
        .init(pubkey: fromPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
        .init(pubkey: toPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
      ],
      data: [],
      decodedData: .init(
        instructionType: UInt32(BraveWallet.SolanaSystemInstruction.createAccount.rawValue),
        accountParams: [
          .init(name: BraveWallet.FromAccount, localizedName: "From Account"),
          .init(name: BraveWallet.NewAccount, localizedName: "New Account"),
        ],
        params: [
          .init(
            name: BraveWallet.Lamports,
            localizedName: "Lamports",
            value: "2000",
            type: .uint64
          ),
          .init(name: "space", localizedName: "Space", value: "1", type: .unknown),
          .init(
            name: "owner_program",
            localizedName: "Owner Program",
            value: toPubkey,
            type: .unknown
          ),
        ]
      )
    )
    let expectedParsedCreateAccount = SolanaTxDetails.ParsedSolanaInstruction(
      name: "System Program - Create Account",
      details: [
        .init(key: "From Account", value: fromPubkey),
        .init(key: "New Account", value: toPubkey),
        .init(key: "Amount", value: "0.000002 SOL"),
        .init(key: "Lamports", value: "2000"),
        .init(key: "Space", value: "1"),
        .init(key: "Owner Program", value: toPubkey),
      ],
      instruction: createAccountInstruction
    )
    XCTAssertNoDifference(
      expectedParsedCreateAccount,
      TransactionParser.parseSolanaInstruction(createAccountInstruction)
    )

    let createAccountWithSeedInstruction = BraveWallet.SolanaInstruction(
      programId: BraveWallet.SolanaSystemProgramId,
      accountMetas: [
        .init(pubkey: fromPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
        .init(pubkey: toPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
      ],
      data: [],
      decodedData: .init(
        instructionType: UInt32(BraveWallet.SolanaSystemInstruction.createAccountWithSeed.rawValue),
        accountParams: [
          .init(name: BraveWallet.FromAccount, localizedName: "From Account"),
          .init(name: "created_account", localizedName: "Created Account"),
        ],
        params: [
          .init(name: BraveWallet.Lamports, localizedName: "Lamports", value: "300", type: .uint64),
          .init(name: "base", localizedName: "Base", value: toPubkey, type: .unknown),
          .init(name: "seed", localizedName: "Seed", value: "", type: .unknown),
          .init(name: "space", localizedName: "Space", value: "1", type: .unknown),
          .init(
            name: "owner_program",
            localizedName: "Owner Program",
            value: toPubkey,
            type: .unknown
          ),
        ]
      )
    )
    let expectedParsedCreateAccountWithSeed = SolanaTxDetails.ParsedSolanaInstruction(
      name: "System Program - Create Account With Seed",
      details: [
        .init(key: "From Account", value: fromPubkey),
        .init(key: "Created Account", value: toPubkey),
        .init(key: "Amount", value: "0.0000003 SOL"),
        .init(key: "Lamports", value: "300"),
        .init(key: "Base", value: toPubkey),
        .init(key: "Seed", value: ""),
        .init(key: "Space", value: "1"),
        .init(key: "Owner Program", value: toPubkey),
      ],
      instruction: createAccountWithSeedInstruction
    )
    XCTAssertNoDifference(
      expectedParsedCreateAccountWithSeed,
      TransactionParser.parseSolanaInstruction(createAccountWithSeedInstruction)
    )
  }

  func testFilecoinSendTransfer() {
    let network: BraveWallet.NetworkInfo = .mockFilecoinTestnet

    let transactionData: BraveWallet.FilTxData = .init(
      nonce: "",
      gasPremium: "100911",
      gasFeeCap: "101965",
      gasLimit: "1527953",
      maxFee: "0",
      to: accountInfos[5].address,
      value: "1000000000000000000"
    )
    let transaction = mockTransaction(
      fromAccount: accountInfos[4],
      txDataUnion: .init(filTxData: transactionData),
      txType: .other,
      chainId: BraveWallet.FilecoinTestnet,
      effectiveRecipient: transactionData.to
    )

    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[4].name,
      fromAccountInfo: accountInfos[4],
      namedToAddress: accountInfos[5].name,
      toAddress: accountInfos[5].address,
      network: .mockFilecoinTestnet,
      details: .filSend(
        .init(
          sendToken: .mockFilToken,
          sendValue: "1000000000000000000",
          sendAmount: "1",
          sendFiat: "$2.00",
          gasPremium: "0.000000000000100911",
          gasLimit: "0.000000000001527953",
          gasFeeCap: "0.000000000000101965",
          gasFee: GasFee(
            fee: "0.000000155797727645",
            fiat: "$0.000000312"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [.mockMainnet, .mockPolygon, .mockFilecoinMainnet, network],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: nil,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse filecoinSendTransfer transaction")
      return
    }

    XCTAssertEqual(
      expectedParsedTransaction.fromAccountInfo.id,
      parsedTransaction.fromAccountInfo.id
    )
    XCTAssertEqual(expectedParsedTransaction.namedFromAddress, parsedTransaction.namedFromAddress)
    XCTAssertEqual(expectedParsedTransaction.toAddress, parsedTransaction.toAddress)
    XCTAssertEqual(expectedParsedTransaction.networkSymbol, parsedTransaction.networkSymbol)
    guard case .filSend(let expectedDetails) = expectedParsedTransaction.details,
      case .filSend(let parsedDetails) = parsedTransaction.details
    else {
      XCTFail("Incorrectly parsed filecoinSendTransfer transaction")
      return
    }

    XCTAssertEqual(expectedDetails.sendValue, parsedDetails.sendValue)
    XCTAssertEqual(expectedDetails.sendAmount, parsedDetails.sendAmount)
    XCTAssertEqual(expectedDetails.sendFiat, parsedDetails.sendFiat)
    XCTAssertEqual(expectedDetails.gasPremium, parsedDetails.gasPremium)
    XCTAssertEqual(expectedDetails.gasLimit, parsedDetails.gasLimit)
    XCTAssertEqual(expectedDetails.gasFeeCap, parsedDetails.gasFeeCap)
    XCTAssertEqual(expectedDetails.gasFee, parsedDetails.gasFee)
  }

  @MainActor func testBitcoinSend() async {
    let network = BraveWallet.NetworkInfo.mockBitcoinMainnet
    let mockFromAccount = accountInfos[6]
    let mockToAccountAddress = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4"

    let transactionData: BraveWallet.BtcTxData = .init(
      to: mockToAccountAddress,
      amount: 5000,
      sendingMaxAmount: false,
      fee: 2544,
      inputs: [],
      outputs: []
    )
    let transaction = mockTransaction(
      fromAccount: mockFromAccount,
      txDataUnion: .init(btcTxData: transactionData),
      txType: .other,
      chainId: network.chainId,
      effectiveRecipient: mockToAccountAddress
    )

    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: mockFromAccount.name,
      fromAccountInfo: mockFromAccount,
      namedToAddress: "",
      toAddress: mockToAccountAddress,
      network: network,
      details: .btcSend(
        .init(
          fromToken: .mockBTCToken,
          fromValue: "5000",
          fromAmount: "0.00005",
          fromFiat: "$3.11",
          fromTokenMetadata: nil,
          gasFee: GasFee(
            fee: "0.00002544",
            fiat: "$1.58"
          )
        )
      )
    )

    guard
      let parsedTransaction = TransactionParser.parseTransaction(
        transaction: transaction,
        allNetworks: [.mockMainnet, .mockPolygon, network],
        accountInfos: accountInfos,
        userAssets: tokens,
        allTokens: tokens,
        assetRatios: assetRatios,
        nftMetadata: [:],
        solEstimatedTxFee: nil,
        currencyFormatter: currencyFormatter
      )
    else {
      XCTFail("Failed to parse btcSend transaction")
      return
    }

    XCTAssertEqual(
      expectedParsedTransaction.fromAccountInfo.id,
      parsedTransaction.fromAccountInfo.id
    )
    XCTAssertEqual(expectedParsedTransaction.namedFromAddress, parsedTransaction.namedFromAddress)
    XCTAssertEqual(expectedParsedTransaction.toAddress, parsedTransaction.toAddress)
    XCTAssertEqual(expectedParsedTransaction.networkSymbol, parsedTransaction.networkSymbol)

    guard case .btcSend(let expectedDetails) = expectedParsedTransaction.details,
      case .btcSend(let parsedDetails) = parsedTransaction.details
    else {
      XCTFail("Incorrectly parsed btcSend transaction")
      return
    }

    XCTAssertEqual(expectedDetails.fromValue, parsedDetails.fromValue)
    XCTAssertEqual(expectedDetails.fromAmount, parsedDetails.fromAmount)
    XCTAssertEqual(expectedDetails.fromFiat, parsedDetails.fromFiat)
    XCTAssertEqual(expectedDetails.gasFee, parsedDetails.gasFee)
  }
}
