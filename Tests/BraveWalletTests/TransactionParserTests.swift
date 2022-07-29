// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveCore
@testable import BraveWallet

private extension BraveWallet.AccountInfo {
  convenience init(
    address: String,
    name: String,
    coin: BraveWallet.CoinType = .eth
  ) {
    self.init(
      address: address,
      name: name,
      isImported: false,
      hardware: nil,
      coin: .eth
    )
  }
}

class TransactionParserTests: XCTestCase {
  
  private let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  private let accountInfos: [BraveWallet.AccountInfo] = [
    .init(address: "0x1234567890123456789012345678901234567890", name: "Ethereum Account 1"),
    .init(address: "0x0987654321098765432109876543210987654321", name: "Ethereum Account 2"),
    .init(address: "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd", name: "Solana Account 1", coin: .sol),
    .init(address: "0xeeeeeeeeeeffffffffff11111111112222222222", name: "Solana Account 2", coin: .sol)
  ]
  private let tokens: [BraveWallet.BlockchainToken] = [
    .previewToken, .previewDaiToken, .mockUSDCToken, .mockSolToken, .mockSpdToken
  ]
  let assetRatios: [String: Double] = ["eth": 1,
                                       "dai": 2,
                                       "usdc": 3,
                                       "sol": 20,
                                       "spd": 15]
  
  func testEthSendTransaction() {
    let network: BraveWallet.NetworkInfo = .mockMainnet
    
    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0x5208",
        to: "0x0987654321098765432109876543210987654321",
        value: "0x1b667a56d488000", // 0.1234
        data: []
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59672ead",
      maxFeePerGas: "0x59672eb6",
      gasEstimation: .init(
        slowMaxPriorityFeePerGas: "0x0",
        slowMaxFeePerGas: "0x9",
        avgMaxPriorityFeePerGas: "0x59672ead",
        avgMaxFeePerGas: "0x59672eb6",
        fastMaxPriorityFeePerGas: "0x59682f00",
        fastMaxFeePerGas: "0x59682f09",
        baseFeePerGas: "0x9"
      )
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "1",
      fromAddress: "0x1234567890123456789012345678901234567890",
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .ethSend,
      txParams: [],
      txArgs: [],
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: "Ethereum Account 1",
      fromAddress: "0x1234567890123456789012345678901234567890",
      namedToAddress: "Ethereum Account 2",
      toAddress: "0x0987654321098765432109876543210987654321",
      networkSymbol: "ETH",
      details: .ethSend(
        .init(
          fromToken: network.nativeToken,
          fromValue: "0x1b667a56d488000",
          fromAmount: "0.1234",
          fromFiat: "$0.12",
          gasFee: .init(
            fee: "0.000031",
            fiat: "$0.000031"
          )
        )
      )
    )
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      visibleTokens: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      solEstimatedTxFee: nil,
      currencyFormatter: currencyFormatter
    ) else {
      XCTFail("Failed to parse ethSend transaction")
      return
    }
    XCTAssertEqual(expectedParsedTransaction.fromAddress, parsedTransaction.fromAddress)
    XCTAssertEqual(expectedParsedTransaction.namedFromAddress, parsedTransaction.namedFromAddress)
    XCTAssertEqual(expectedParsedTransaction.toAddress, parsedTransaction.toAddress)
    XCTAssertEqual(expectedParsedTransaction.networkSymbol, parsedTransaction.networkSymbol)
    guard case let .ethSend(expectedDetails) = expectedParsedTransaction.details,
          case let .ethSend(parsedDetails) = parsedTransaction.details else {
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
        data: []
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59672ead",
      maxFeePerGas: "0x59672eb6",
      gasEstimation: .init(
        slowMaxPriorityFeePerGas: "0x0",
        slowMaxFeePerGas: "0x9",
        avgMaxPriorityFeePerGas: "0x59672ead",
        avgMaxFeePerGas: "0x59672eb6",
        fastMaxPriorityFeePerGas: "0x59682f00",
        fastMaxFeePerGas: "0x59682f09",
        baseFeePerGas: "0x9"
      )
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "2",
      fromAddress: "0x1234567890123456789012345678901234567890",
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .erc20Transfer,
      txParams: [],
      txArgs: ["0x0987654321098765432109876543210987654321", "0x5ff20a91f724000"], // toAddress, 0.4321
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: "Ethereum Account 1",
      fromAddress: "0x1234567890123456789012345678901234567890",
      namedToAddress: "Ethereum Account 2",
      toAddress: "0x0987654321098765432109876543210987654321",
      networkSymbol: "ETH",
      details: .erc20Transfer(
        .init(
          fromToken: .previewDaiToken,
          fromValue: "0x5ff20a91f724000",
          fromAmount: "0.4321",
          fromFiat: "$0.86",
          gasFee: .init(
            fee: "0.000078",
            fiat: "$0.000078"
          )
        )
      )
    )
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      visibleTokens: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      solEstimatedTxFee: nil,
      currencyFormatter: currencyFormatter
    ) else {
      XCTFail("Failed to parse erc20Transfer transaction")
      return
    }
    XCTAssertEqual(expectedParsedTransaction, parsedTransaction)
  }
  
  func testEthSwapTransaction() {
    let network: BraveWallet.NetworkInfo = .mockMainnet
    
    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0x4be75",
        to: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF", // 0x exchange address
        value: "0x1b6951ef585a000",
        data: []
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59682f00",
      maxFeePerGas: "0x59682f09",
      gasEstimation: .init(
        slowMaxPriorityFeePerGas: "0x4ed3152b",
        slowMaxFeePerGas: "0x4ed31534",
        avgMaxPriorityFeePerGas: "0x59672ead",
        avgMaxFeePerGas: "0x59672eb6",
        fastMaxPriorityFeePerGas: "0x59682f00",
        fastMaxFeePerGas: "0x59682f09",
        baseFeePerGas: "0x9"
      )
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "3",
      fromAddress: "0x1234567890123456789012345678901234567890",
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .ethSwap,
      txParams: [],
      txArgs: [
        "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeead6d458402f60fd3bd25163575031acdce07538d", // eth -> dai
        "0x1b6951ef585a000", // 0.12345 eth
        "0x5c6f2d76e910358b" // 6.660592362643797387 dai
      ],
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: "Ethereum Account 1",
      fromAddress: "0x1234567890123456789012345678901234567890",
      namedToAddress: "0x Exchange Proxy",
      toAddress: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF",
      networkSymbol: "ETH",
      details: .ethSwap(
        .init(
          fromToken: .previewToken,
          fromValue: "0x1b6951ef585a000",
          fromAmount: "0.12345",
          fromFiat: "$0.12",
          toToken: .previewDaiToken,
          minBuyValue: "0x5c6f2d76e910358b",
          minBuyAmount: "6.660592362643797387",
          minBuyAmountFiat: "$13.32",
          gasFee: .init(
            fee: "0.000466",
            fiat: "$0.000466"
          )
        )
      )
    )
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      visibleTokens: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      solEstimatedTxFee: nil,
      currencyFormatter: currencyFormatter
    ) else {
      XCTFail("Failed to parse ethSwap transaction")
      return
    }
    XCTAssertEqual(expectedParsedTransaction, parsedTransaction)
  }
  
  func testEthSwapTransactionUSDCToDAI() {
    let network: BraveWallet.NetworkInfo = .mockMainnet
    
    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0x4be75",
        to: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF", // 0x exchange address
        value: "0x0",
        data: []
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59682f00",
      maxFeePerGas: "0x59682f09",
      gasEstimation: .init(
        slowMaxPriorityFeePerGas: "0x4ed3152b",
        slowMaxFeePerGas: "0x4ed31534",
        avgMaxPriorityFeePerGas: "0x59672ead",
        avgMaxFeePerGas: "0x59672eb6",
        fastMaxPriorityFeePerGas: "0x59682f00",
        fastMaxFeePerGas: "0x59682f09",
        baseFeePerGas: "0x9"
      )
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "3",
      fromAddress: "0x1234567890123456789012345678901234567890",
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .ethSwap,
      txParams: [],
      txArgs: [
        "0x07865c6e87b9f70255377e024ace6630c1eaa37fad6d458402f60fd3bd25163575031acdce07538d", // usdc -> dai
        "0x16e360", // 1.5 USDC
        "0x1bd02ca9a7c244e" // ~0.1253 DAI
      ],
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: "Ethereum Account 1",
      fromAddress: "0x1234567890123456789012345678901234567890",
      namedToAddress: "0x Exchange Proxy",
      toAddress: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF",
      networkSymbol: "ETH",
      details: .ethSwap(
        .init(
          fromToken: .mockUSDCToken,
          fromValue: "0x16e360",
          fromAmount: "1.5",
          fromFiat: "$4.50",
          toToken: .previewDaiToken,
          minBuyValue: "0x1bd02ca9a7c244e",
          minBuyAmount: "0.125259433834718286",
          minBuyAmountFiat: "$0.25",
          gasFee: .init(
            fee: "0.000466",
            fiat: "$0.000466"
          )
        )
      )
    )
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      visibleTokens: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      solEstimatedTxFee: nil,
      currencyFormatter: currencyFormatter
    ) else {
      XCTFail("Failed to parse ethSwap transaction")
      return
    }
    XCTAssertEqual(expectedParsedTransaction, parsedTransaction)
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
        data: []
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59682f00",
      maxFeePerGas: "0x59682f09",
      gasEstimation: .init(
        slowMaxPriorityFeePerGas: "0x0",
        slowMaxFeePerGas: "0x9",
        avgMaxPriorityFeePerGas: "0x59672ead",
        avgMaxFeePerGas: "0x59672eb6",
        fastMaxPriorityFeePerGas: "0x59682f00",
        fastMaxFeePerGas: "0x59682f09",
        baseFeePerGas: "0x9"
      )
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "4",
      fromAddress: "0x1234567890123456789012345678901234567890",
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .erc20Approve,
      txParams: [],
      txArgs: ["", "0x2386f26fc10000"], // 0.01
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: "Ethereum Account 1",
      fromAddress: "0x1234567890123456789012345678901234567890",
      namedToAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress.truncatedAddress,
      toAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
      networkSymbol: "ETH",
      details: .ethErc20Approve(
        .init(
          token: .previewDaiToken,
          approvalValue: "0x2386f26fc10000",
          approvalAmount: "0.01",
          isUnlimited: false,
          spenderAddress: "",
          gasFee: .init(
            fee: "0.000061",
            fiat: "$0.000061"
          )
        )
      )
    )
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      visibleTokens: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      solEstimatedTxFee: nil,
      currencyFormatter: currencyFormatter
    ) else {
      XCTFail("Failed to parse erc20Approve transaction")
      return
    }
    XCTAssertEqual(expectedParsedTransaction, parsedTransaction)
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
        data: []
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59682f00",
      maxFeePerGas: "0x59682f09",
      gasEstimation: .init(
        slowMaxPriorityFeePerGas: "0x0",
        slowMaxFeePerGas: "0x9",
        avgMaxPriorityFeePerGas: "0x59672ead",
        avgMaxFeePerGas: "0x59672eb6",
        fastMaxPriorityFeePerGas: "0x59682f00",
        fastMaxFeePerGas: "0x59682f09",
        baseFeePerGas: "0x9"
      )
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "5",
      fromAddress: "0x1234567890123456789012345678901234567890",
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .erc20Approve,
      txParams: [],
      txArgs: ["", "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"], // unlimited
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: "Ethereum Account 1",
      fromAddress: "0x1234567890123456789012345678901234567890",
      namedToAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress.truncatedAddress,
      toAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
      networkSymbol: "ETH",
      details: .ethErc20Approve(
        .init(
          token: .previewDaiToken,
          approvalValue: "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
          approvalAmount: "Unlimited",
          isUnlimited: true,
          spenderAddress: "",
          gasFee: .init(
            fee: "0.000061",
            fiat: "$0.000061"
          )
        )
      )
    )
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      visibleTokens: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      solEstimatedTxFee: nil,
      currencyFormatter: currencyFormatter
    ) else {
      XCTFail("Failed to parse erc20Approve transaction")
      return
    }
    XCTAssertEqual(expectedParsedTransaction, parsedTransaction)
  }
  
  func testErc721TransferFromTransaction() {
    let network: BraveWallet.NetworkInfo = .mockMainnet
    
    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0x5208",
        to: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
        value: "0xde0b6b3a7640000", // 1
        data: []
      ),
      chainId: network.chainId,
      maxPriorityFeePerGas: "0x59672ead",
      maxFeePerGas: "0x59672eb6",
      gasEstimation: .init(
        slowMaxPriorityFeePerGas: "0x0",
        slowMaxFeePerGas: "0x9",
        avgMaxPriorityFeePerGas: "0x59672ead",
        avgMaxFeePerGas: "0x59672eb6",
        fastMaxPriorityFeePerGas: "0x59682f00",
        fastMaxFeePerGas: "0x59682f09",
        baseFeePerGas: "0x9"
      )
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "6",
      fromAddress: "0x1234567890123456789012345678901234567890",
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .erc721TransferFrom,
      txParams: [],
      txArgs: [
        "0x1111111111aaaaaaaaaa2222222222bbbbbbbbbb", // owner
        "0x0987654321098765432109876543210987654321", // toAddress
        "token.id"
      ],
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: "Ethereum Account 1",
      fromAddress: "0x1234567890123456789012345678901234567890",
      namedToAddress: "Ethereum Account 2",
      toAddress: "0x0987654321098765432109876543210987654321",
      networkSymbol: "ETH",
      details: .erc721Transfer(
        .init(
          fromToken: .previewDaiToken,
          fromValue: "1",
          fromAmount: "1",
          owner: "0x1111111111aaaaaaaaaa2222222222bbbbbbbbbb",
          tokenId: "token.id"
        )
      )
    )
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      visibleTokens: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      solEstimatedTxFee: nil,
      currencyFormatter: currencyFormatter
    ) else {
      XCTFail("Failed to parse erc721TransferFrom transaction")
      return
    }
    XCTAssertEqual(expectedParsedTransaction, parsedTransaction)
  }
  
  func testSolanaSystemTransfer() {
    let network: BraveWallet.NetworkInfo = .mockSolana
    
    let transactionData: BraveWallet.SolanaTxData = .init(
      recentBlockhash: "",
      lastValidBlockHeight: 0,
      feePayer: "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd",
      toWalletAddress: "0xeeeeeeeeeeffffffffff11111111112222222222",
      splTokenMintAddress: "",
      lamports: 100000000,
      amount: 0,
      txType: .solanaSystemTransfer,
      instructions: [
        .init(
          programId: "",
          accountMetas: [.init(pubkey: "", isSigner: false, isWritable: false)],
          data: [])
      ],
      send: .init(maxRetries: .init(maxRetries: 1), preflightCommitment: nil, skipPreflight: nil),
      signTransactionParam: nil
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "7",
      fromAddress: "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd",
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(solanaTxData: transactionData),
      txStatus: .confirmed,
      txType: .solanaSystemTransfer,
      txParams: [],
      txArgs: [
      ],
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil
    )
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: "Solana Account 1",
      fromAddress: "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd",
      namedToAddress: "Solana Account 2",
      toAddress: "0xeeeeeeeeeeffffffffff11111111112222222222",
      networkSymbol: "SOL",
      details: .solSystemTransfer(
        .init(
          fromToken: .mockSolToken,
          fromValue: "100000000",
          fromAmount: "0.1",
          fromFiat: "$2.00",
          gasFee: .init(
            fee: "0.00123",
            fiat: "$0.0246"
          )
        )
      )
    )
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      visibleTokens: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      solEstimatedTxFee: 1230000,
      currencyFormatter: currencyFormatter
    ) else {
      XCTFail("Failed to parse solanaSystemTransfer transaction")
      return
    }
    
    XCTAssertEqual(expectedParsedTransaction.fromAddress, parsedTransaction.fromAddress)
    XCTAssertEqual(expectedParsedTransaction.namedFromAddress, parsedTransaction.namedFromAddress)
    XCTAssertEqual(expectedParsedTransaction.toAddress, parsedTransaction.toAddress)
    XCTAssertEqual(expectedParsedTransaction.networkSymbol, parsedTransaction.networkSymbol)
    guard case let .solSystemTransfer(expectedDetails) = expectedParsedTransaction.details,
          case let .solSystemTransfer(parsedDetails) = parsedTransaction.details else {
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
      feePayer: "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd",
      toWalletAddress: "0xeeeeeeeeeeffffffffff11111111112222222222",
      splTokenMintAddress: BraveWallet.BlockchainToken.mockSpdToken.contractAddress,
      lamports: 0,
      amount: 43210000,
      txType: .solanaSplTokenTransfer,
      instructions: [
        .init(
          programId: "",
          accountMetas: [.init(pubkey: "", isSigner: false, isWritable: false)],
          data: [])
      ],
      send: .init(maxRetries: .init(maxRetries: 1), preflightCommitment: nil, skipPreflight: nil),
      signTransactionParam: nil
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "7",
      fromAddress: "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd",
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(solanaTxData: transactionData),
      txStatus: .confirmed,
      txType: .solanaSplTokenTransfer,
      txParams: [],
      txArgs: [
      ],
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil
    )
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: "Solana Account 1",
      fromAddress: "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd",
      namedToAddress: "Solana Account 2",
      toAddress: "0xeeeeeeeeeeffffffffff11111111112222222222",
      networkSymbol: "SOL",
      details: .solSplTokenTransfer(
        .init(
          fromToken: .mockSpdToken,
          fromValue: "43210000",
          fromAmount: "43.21",
          fromFiat: "$648.15",
          gasFee: .init(
            fee: "0.0123",
            fiat: "$0.246"
          )
        )
      )
    )
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      visibleTokens: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      solEstimatedTxFee: 12300000,
      currencyFormatter: currencyFormatter
    ) else {
      XCTFail("Failed to parse solanaSplTokenTransfer transaction")
      return
    }
    
    XCTAssertEqual(expectedParsedTransaction, parsedTransaction)
  }
}
