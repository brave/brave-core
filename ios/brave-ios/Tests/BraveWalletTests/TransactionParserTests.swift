// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveCore
@testable import BraveWallet
import CustomDump

private extension BraveWallet.AccountInfo {
  convenience init(
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
        bitcoinAccountIndex: 0,
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
      $0.accountId.address = "0x0987654321098765432109876543210987654321"
    },
    (BraveWallet.AccountInfo.mockSolAccount.copy() as! BraveWallet.AccountInfo).then {
      $0.name = "Solana Account 1"
      $0.address = "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd"
      $0.accountId.address = "0xaaaaaaaaaabbbbbbbbbbccccccccccdddddddddd"
    },
    (BraveWallet.AccountInfo.mockSolAccount.copy() as! BraveWallet.AccountInfo).then {
      $0.name = "Solana Account 2"
      $0.address = "0xeeeeeeeeeeffffffffff11111111112222222222"
      $0.accountId.address = "0xeeeeeeeeeeffffffffff11111111112222222222"
    },
    (BraveWallet.AccountInfo.mockFilTestnetAccount.copy() as! BraveWallet.AccountInfo).then {
      $0.name = "Filecoin Testnet 1"
      $0.address = "fil_testnet_address_1"
      $0.accountId.address = "fil_testnet_address_1"
    },
    (BraveWallet.AccountInfo.mockFilTestnetAccount.copy() as! BraveWallet.AccountInfo).then {
      $0.name = "Filecoin Testnet 2"
      $0.address = "fil_testnet_address_2"
      $0.accountId.address = "fil_testnet_address_2"
    }
  ]
  private let tokens: [BraveWallet.BlockchainToken] = [
    .previewToken, .previewDaiToken, .mockUSDCToken, .mockSolToken, .mockSpdToken, .mockSolanaNFTToken, .mockFilToken
  ]
  let assetRatios: [String: Double] = [
    "eth": 1,
    BraveWallet.BlockchainToken.previewDaiToken.assetRatioId.lowercased(): 2,
    BraveWallet.BlockchainToken.mockUSDCToken.assetRatioId.lowercased(): 3,
    "sol": 20,
    BraveWallet.BlockchainToken.mockSpdToken.assetRatioId.lowercased(): 15,
    "fil": 2
  ]
  
  func testEthSendTransaction() {
    let network: BraveWallet.NetworkInfo = .mockMainnet
    
    let transactionData: BraveWallet.TxData1559 = .init(
      baseData: .init(
        nonce: "1",
        gasPrice: "0x0",
        gasLimit: "0x5208",
        to: "0x0987654321098765432109876543210987654321",
        value: "0x1b667a56d488000", // 0.1234
        data: [],
        signOnly: false,
        signedTransaction: nil
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
      fromAddress: accountInfos[0].accountId.address,
      from: accountInfos[0].accountId,
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .ethSend,
      txParams: [],
      txArgs: [],
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil,
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: transactionData.baseData.to
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAddress: accountInfos[0].address,
      namedToAddress: "Ethereum Account 2",
      toAddress: "0x0987654321098765432109876543210987654321",
      network: .mockMainnet,
      details: .ethSend(
        .init(
          fromToken: network.nativeToken,
          fromValue: "0x1b667a56d488000",
          fromAmount: "0.1234",
          fromFiat: "$0.12",
          fromTokenMetadata: nil,
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
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
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
        data: [],
        signOnly: false,
        signedTransaction: nil
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
      fromAddress: accountInfos[0].address,
      from: accountInfos[0].accountId,
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .erc20Transfer,
      txParams: [],
      txArgs: ["0x0987654321098765432109876543210987654321", "0x5ff20a91f724000"], // toAddress, 0.4321
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil,
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: transactionData.baseData.to
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAddress: accountInfos[0].address,
      namedToAddress: "Ethereum Account 2",
      toAddress: "0x0987654321098765432109876543210987654321",
      network: .mockMainnet,
      details: .erc20Transfer(
        .init(
          fromToken: .previewDaiToken,
          fromValue: "0x5ff20a91f724000",
          fromAmount: "0.4321",
          fromFiat: "$0.86",
          fromTokenMetadata: nil,
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
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
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
        data: [],
        signOnly: false,
        signedTransaction: nil
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
      fromAddress: accountInfos[0].address,
      from: accountInfos[0].accountId,
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
      originInfo: nil,
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: transactionData.baseData.to
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAddress: accountInfos[0].address,
      namedToAddress: "0x Exchange Proxy",
      toAddress: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF",
      network: .mockMainnet,
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
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
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
        data: [],
        signOnly: false,
        signedTransaction: nil
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
      fromAddress: accountInfos[0].address,
      from: accountInfos[0].accountId,
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
      originInfo: nil,
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: transactionData.baseData.to
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAddress: accountInfos[0].address,
      namedToAddress: "0x Exchange Proxy",
      toAddress: "0xDef1C0ded9bec7F1a1670819833240f027b25EfF",
      network: .mockMainnet,
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
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
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
        data: [],
        signOnly: false,
        signedTransaction: nil
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
      fromAddress: accountInfos[0].address,
      from: accountInfos[0].accountId,
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .erc20Approve,
      txParams: [],
      txArgs: ["", "0x2386f26fc10000"], // 0.01
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil,
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: transactionData.baseData.to
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAddress: accountInfos[0].address,
      namedToAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress.truncatedAddress,
      toAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
      network: .mockMainnet,
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
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
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
        data: [],
        signOnly: false,
        signedTransaction: nil
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
      fromAddress: accountInfos[0].address,
      from: accountInfos[0].accountId,
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff1234",
      txDataUnion: .init(ethTxData1559: transactionData),
      txStatus: .confirmed,
      txType: .erc20Approve,
      txParams: [],
      txArgs: ["", "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"], // unlimited
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil,
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: transactionData.baseData.to
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAddress: accountInfos[0].address,
      namedToAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress.truncatedAddress,
      toAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
      network: .mockMainnet,
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
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
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
        data: [],
        signOnly: false,
        signedTransaction: nil
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
      fromAddress: accountInfos[0].address,
      from: accountInfos[0].accountId,
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
      originInfo: nil,
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: transactionData.baseData.to
    )
    
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[0].name,
      fromAddress: accountInfos[0].address,
      namedToAddress: "Ethereum Account 2",
      toAddress: "0x0987654321098765432109876543210987654321",
      network: .mockMainnet,
      details: .erc721Transfer(
        .init(
          fromToken: .previewDaiToken,
          fromValue: "1",
          fromAmount: "1",
          nftMetadata: nil,
          owner: "0x1111111111aaaaaaaaaa2222222222bbbbbbbbbb",
          tokenId: "token.id",
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
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
      solEstimatedTxFee: nil,
      currencyFormatter: currencyFormatter
    ) else {
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
      feePayer: accountInfos[2].accountId.address,
      toWalletAddress: accountInfos[3].accountId.address,
      splTokenMintAddress: "",
      lamports: 100000000,
      amount: 0,
      txType: .solanaSystemTransfer,
      instructions: [
        .init(
          programId: "",
          accountMetas: [
            .init(pubkey: "", addrTableLookupIndex: nil, isSigner: false, isWritable: false)
          ],
          data: [],
          decodedData: nil)
      ],
      version: .legacy,
      messageHeader: .init(),
      staticAccountKeys: [],
      addressTableLookups: [],
      send: .init(maxRetries: .init(maxRetries: 1), preflightCommitment: nil, skipPreflight: nil),
      signTransactionParam: nil
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "7",
      fromAddress: accountInfos[2].accountId.address,
      from: accountInfos[2].accountId,
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
      originInfo: nil,
      chainId: BraveWallet.SolanaMainnet,
      effectiveRecipient: nil
    )
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[2].name,
      fromAddress: accountInfos[2].accountId.address,
      namedToAddress: accountInfos[3].name,
      toAddress: accountInfos[3].accountId.address,
      network: .mockSolana,
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
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
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
      feePayer: accountInfos[2].accountId.address,
      toWalletAddress: accountInfos[3].accountId.address,
      splTokenMintAddress: BraveWallet.BlockchainToken.mockSpdToken.contractAddress,
      lamports: 0,
      amount: 43210000,
      txType: .solanaSplTokenTransfer,
      instructions: [
        .init(
          programId: "",
          accountMetas: [
            .init(pubkey: "", addrTableLookupIndex: nil, isSigner: false, isWritable: false)
          ],
          data: [],
          decodedData: nil)
      ],
      version: .legacy,
      messageHeader: .init(),
      staticAccountKeys: [],
      addressTableLookups: [],
      send: .init(maxRetries: .init(maxRetries: 1), preflightCommitment: nil, skipPreflight: nil),
      signTransactionParam: nil
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "7",
      fromAddress: accountInfos[2].accountId.address,
      from: accountInfos[2].accountId,
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
      originInfo: nil,
      chainId: BraveWallet.SolanaMainnet,
      effectiveRecipient: nil
    )
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[2].name,
      fromAddress: accountInfos[2].accountId.address,
      namedToAddress: accountInfos[3].name,
      toAddress: accountInfos[3].accountId.address,
      network: .mockSolana,
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
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
      solEstimatedTxFee: 12300000,
      currencyFormatter: currencyFormatter
    ) else {
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
      feePayer: accountInfos[2].accountId.address,
      toWalletAddress: accountInfos[3].accountId.address,
      splTokenMintAddress: BraveWallet.BlockchainToken.mockSolanaNFTToken.contractAddress,
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
          decodedData: nil)
      ],
      version: .legacy,
      messageHeader: .init(),
      staticAccountKeys: [],
      addressTableLookups: [],
      send: .init(maxRetries: .init(maxRetries: 1), preflightCommitment: nil, skipPreflight: nil),
      signTransactionParam: nil
    )
    let transaction = BraveWallet.TransactionInfo(
      id: "7",
      fromAddress: accountInfos[2].accountId.address,
      from: accountInfos[2].accountId,
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
      originInfo: nil,
      chainId: BraveWallet.SolanaMainnet,
      effectiveRecipient: nil
    )
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[2].name,
      fromAddress: accountInfos[2].accountId.address,
      namedToAddress: accountInfos[3].name,
      toAddress: accountInfos[3].accountId.address,
      network: .mockSolana,
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
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
      solEstimatedTxFee: 12300000,
      currencyFormatter: currencyFormatter
    ) else {
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
        .init(pubkey: toPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false)
      ],
      data: [],
      decodedData: .init(
        instructionType: UInt32(BraveWallet.SolanaSystemInstruction.transfer.rawValue),
        accountParams: [
          .init(name: BraveWallet.FromAccount, localizedName: "From Account"),
          .init(name: BraveWallet.ToAccount, localizedName: "To Account"),],
        params: [.init(name: BraveWallet.Lamports, localizedName: "Lamports", value: "10000", type: .uint64)]
      )
    )
    let expectedParsedTransfer = SolanaTxDetails.ParsedSolanaInstruction(
      name: "System Program - Transfer",
      details: [
        .init(key: "From Account", value: fromPubkey),
        .init(key: "To Account", value: toPubkey),
        .init(key: "Amount", value: "0.00001 SOL"),
        .init(key: "Lamports", value: "10000")
      ],
      instruction: transferInstruction
    )
    XCTAssertNoDifference(expectedParsedTransfer, TransactionParser.parseSolanaInstruction(transferInstruction))
    
    let withdrawNonceAccountInstruction = BraveWallet.SolanaInstruction(
      programId: BraveWallet.SolanaSystemProgramId,
      accountMetas: [
        .init(pubkey: fromPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
        .init(pubkey: toPubkey, addrTableLookupIndex: nil, isSigner: false, isWritable: false),
        .init(pubkey: "SysvarRecentB1ockHashes11111111111111111111", addrTableLookupIndex: nil, isSigner: false, isWritable: false),
        .init(pubkey: "SysvarRent111111111111111111111111111111111", addrTableLookupIndex: nil, isSigner: false, isWritable: false),
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
          .init(name: "nonce_authority", localizedName: "Nonce Authority")
        ],
        params: [
          .init(name: BraveWallet.Lamports, localizedName: "Lamports", value: "40", type: .uint64)
        ]
      )
    )
    let expectedParsedWithdrawNonceAccount = SolanaTxDetails.ParsedSolanaInstruction(
      name: "System Program - Withdraw Nonce Account",
      details: [
        .init(key: "Nonce Account", value: fromPubkey),
        .init(key: "To Account", value: toPubkey),
        .init(key: "RecentBlockhashes sysvar", value: "SysvarRecentB1ockHashes11111111111111111111"),
        .init(key: "Rent sysvar", value: "SysvarRent111111111111111111111111111111111"),
        .init(key: "Nonce Authority", value: toPubkey),
        .init(key: "Amount", value: "0.00000004 SOL"),
        .init(key: "Lamports", value: "40")
      ],
      instruction: withdrawNonceAccountInstruction
    )
    XCTAssertNoDifference(expectedParsedWithdrawNonceAccount, TransactionParser.parseSolanaInstruction(withdrawNonceAccountInstruction))
    
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
          .init(name: BraveWallet.Lamports, localizedName: "Lamports", value: "2000", type: .uint64),
          .init(name: "space", localizedName: "Space", value: "1", type: .unknown),
          .init(name: "owner_program", localizedName: "Owner Program", value: toPubkey, type: .unknown)
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
        .init(key: "Owner Program", value: toPubkey)
      ],
      instruction: createAccountInstruction
    )
    XCTAssertNoDifference(expectedParsedCreateAccount, TransactionParser.parseSolanaInstruction(createAccountInstruction))
    
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
          .init(name: "owner_program", localizedName: "Owner Program", value: toPubkey, type: .unknown)
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
        .init(key: "Owner Program", value: toPubkey)
      ],
      instruction: createAccountWithSeedInstruction
    )
    XCTAssertNoDifference(expectedParsedCreateAccountWithSeed, TransactionParser.parseSolanaInstruction(createAccountWithSeedInstruction))
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
    let transaction = BraveWallet.TransactionInfo(
      id: "8",
      fromAddress: accountInfos[4].address,
      from: accountInfos[4].accountId,
      txHash: "0xaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffggggg1234",
      txDataUnion: .init(filTxData: transactionData),
      txStatus: .unapproved,
      txType: .other,
      txParams: [],
      txArgs: [
      ],
      createdTime: Date(),
      submittedTime: Date(),
      confirmedTime: Date(),
      originInfo: nil,
      chainId: BraveWallet.FilecoinTestnet,
      effectiveRecipient: nil
    )
  
    let expectedParsedTransaction = ParsedTransaction(
      transaction: transaction,
      namedFromAddress: accountInfos[4].name,
      fromAddress: accountInfos[4].address,
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
            fiat: "$0.0000003116"
          )
        )
      )
    )
    
    guard let parsedTransaction = TransactionParser.parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      userAssets: tokens,
      allTokens: tokens,
      assetRatios: assetRatios,
      nftMetadata: [:],
      solEstimatedTxFee: nil,
      currencyFormatter: currencyFormatter
    ) else {
      XCTFail("Failed to parse filecoinSendTransfer transaction")
      return
    }
    
    XCTAssertEqual(expectedParsedTransaction.fromAddress, parsedTransaction.fromAddress)
    XCTAssertEqual(expectedParsedTransaction.namedFromAddress, parsedTransaction.namedFromAddress)
    XCTAssertEqual(expectedParsedTransaction.toAddress, parsedTransaction.toAddress)
    XCTAssertEqual(expectedParsedTransaction.networkSymbol, parsedTransaction.networkSymbol)
    guard case let .filSend(expectedDetails) = expectedParsedTransaction.details,
          case let .filSend(parsedDetails) = parsedTransaction.details else {
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
}
