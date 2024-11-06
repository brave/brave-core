// swift-format-ignore-file

// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

#if DEBUG

extension BraveWallet.BlockchainToken {
  static let previewToken: BraveWallet.BlockchainToken = .init(
    contractAddress: "",
    name: "Ethereum",
    logo: "",
    isCompressed: false,
    isErc20: false,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unsupported,
    isNft: false,
    isSpam: false,
    symbol: "ETH",
    decimals: 18,
    visible: false,
    tokenId: "",
    coingeckoId: "",
    chainId: BraveWallet.MainnetChainId,
    coin: .eth,
    isShielded: false
  )

  static let previewDaiToken: BraveWallet.BlockchainToken = .init(
    contractAddress: "0xad6d458402f60fd3bd25163575031acdce07538d",
    name: "Dai Stablecoin",
    logo: "",
    isCompressed: false,
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unsupported,
    isNft: false,
    isSpam: false,
    symbol: "DAI",
    decimals: 18,
    visible: false,
    tokenId: "",
    coingeckoId: "",
    chainId: BraveWallet.MainnetChainId,
    coin: .eth,
    isShielded: false
  )

  static let mockUSDCToken: BraveWallet.BlockchainToken = .init(
    contractAddress: "0x07865c6e87b9f70255377e024ace6630c1eaa37f",
    name: "USDC",
    logo: "",
    isCompressed: false,
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unsupported,
    isNft: false,
    isSpam: false,
    symbol: "USDC",
    decimals: 6,
    visible: false,
    tokenId: "",
    coingeckoId: "",
    chainId: BraveWallet.MainnetChainId,
    coin: .eth,
    isShielded: false
  )

  static let mockSolToken: BraveWallet.BlockchainToken = .init(
    contractAddress: "",
    name: "Solana",
    logo: "",
    isCompressed: false,
    isErc20: false,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unsupported,
    isNft: false,
    isSpam: false,
    symbol: "SOL",
    decimals: 9,
    visible: false,
    tokenId: "",
    coingeckoId: "",
    chainId: BraveWallet.SolanaMainnet,
    coin: .sol,
    isShielded: false
  )

  static let mockSpdToken: BraveWallet.BlockchainToken = .init(
    contractAddress: "0x1111111111222222222233333333334444444444",
    name: "Solpad",
    logo: "",
    isCompressed: false,
    isErc20: false,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unknown,
    isNft: false,
    isSpam: false,
    symbol: "SPD",
    decimals: 6,
    visible: false,
    tokenId: "",
    coingeckoId: "",
    chainId: BraveWallet.SolanaMainnet,
    coin: .sol,
    isShielded: false
  )

  static let mockERC721NFTToken: BraveWallet.BlockchainToken = .init(
    contractAddress: "0xaaaaaaaaaa222222222233333333334444444444",
    name: "XENFT",
    logo: "",
    isCompressed: false,
    isErc20: false,
    isErc721: true,
    isErc1155: false,
    splTokenProgram: .unsupported,
    isNft: true,
    isSpam: false,
    symbol: "XENFT",
    decimals: 0,
    visible: true,
    tokenId: "30934",
    coingeckoId: "",
    chainId: BraveWallet.MainnetChainId,
    coin: .eth,
    isShielded: false
  )

  static let mockSolanaNFTToken: BraveWallet.BlockchainToken = .init(
    contractAddress: "aaaaaaaaaa222222222233333333334444444444",
    name: "SOLNFT",
    logo: "",
    isCompressed: false,
    isErc20: false,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unknown,
    isNft: true,
    isSpam: false,
    symbol: "SOLNFT",
    decimals: 0,
    visible: true,
    tokenId: "",
    coingeckoId: "",
    chainId: BraveWallet.SolanaMainnet,
    coin: .sol,
    isShielded: false
  )

  static let mockFilToken: BraveWallet.BlockchainToken = .init(
    contractAddress: "",
    name: "Filcoin",
    logo: "",
    isCompressed: false,
    isErc20: false,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unsupported,
    isNft: false,
    isSpam: false,
    symbol: "FIL",
    decimals: 18,
    visible: false,
    tokenId: "",
    coingeckoId: "",
    chainId: BraveWallet.FilecoinMainnet,
    coin: .fil,
    isShielded: false
  )

  static let mockBTCToken: BraveWallet.BlockchainToken = .init(
    contractAddress: "",
    name: "Bitcoin",
    logo: "",
    isCompressed: false,
    isErc20: false,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unsupported,
    isNft: false,
    isSpam: false,
    symbol: "BTC",
    decimals: 8,
    visible: false,
    tokenId: "",
    coingeckoId: "",
    chainId: BraveWallet.BitcoinMainnet,
    coin: .btc,
    isShielded: false
  )
}

extension BraveWallet.AccountInfo {
  static var previewAccount: BraveWallet.AccountInfo {
    .init(
      accountId: .init(
        coin: .eth,
        keyringId: BraveWallet.KeyringId.default,
        kind: .derived,
        address: "0x879240B2D6179E9EC40BC2AFFF9E9EC40BC2AFFF",
        accountIndex: 0,
        uniqueKey: "0x879240B2D6179E9EC40BC2AFFF9E9EC40BC2AFFF"
      ),
      address: "0x879240B2D6179E9EC40BC2AFFF9E9EC40BC2AFFF",
      name: "Account 1",
      hardware: nil
    )
  }
}

extension BraveWallet.TransactionInfo {
  static var previewConfirmedSend: BraveWallet.TransactionInfo {
    BraveWallet.TransactionInfo(
      id: "fce43e63-1f68-4685-9d40-035f13250a4c",
      fromAddress: BraveWallet.AccountInfo.previewAccount.accountId.address,
      from: BraveWallet.AccountInfo.previewAccount.accountId,
      txHash: "0x46fbd9d5ed775b9e5836aacaf0ed7a78bf5f5a4da451f23238c6123ed0fd51bf",
      txDataUnion: .init(
        ethTxData1559: .init(
          baseData: .init(
            nonce: "0x6",
            gasPrice: "0x0",
            gasLimit: "0x5208",
            to: "0x3f2116ef98fcab1a9c3c2d8988e0064ab59acfca",
            value: "0x2386f26fc10000",
            data: [],
            signOnly: false,
            signedTransaction: nil
          ),
          chainId: BraveWallet.MainnetChainId,
          maxPriorityFeePerGas: "0x2540be400",
          maxFeePerGas: "0x25b7f3d400",
          gasEstimation: nil
        )
      ),
      txStatus: .confirmed,
      txType: .ethSend,
      txParams: [],
      txArgs: [],
      createdTime: Date(timeIntervalSince1970: 1_636_399_671),  // Monday, November 8, 2021 7:27:51 PM
      submittedTime: Date(timeIntervalSince1970: 1_636_399_673),  // Monday, November 8, 2021 7:27:53 PM
      confirmedTime: Date(timeIntervalSince1970: 1_636_402_508),  // Monday, November 8, 2021 8:15:08 PM
      originInfo: .init(),
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: "0x3f2116ef98fcab1a9c3c2d8988e0064ab59acfca",
      isRetriable: false,
      swapInfo: nil
    )
  }
  static var previewConfirmedSwap: BraveWallet.TransactionInfo {
    BraveWallet.TransactionInfo(
      id: "2531db97-6d1d-4906-a1b2-f829c41f489e",
      fromAddress: BraveWallet.AccountInfo.previewAccount.accountId.address,
      from: BraveWallet.AccountInfo.previewAccount.accountId,
      txHash: "0xe21f7110753a8a42793c0b6c0c649aac1545488e57a3f57541b9f199d6b2be11",
      txDataUnion: .init(
        ethTxData1559: .init(
          baseData: .init(
            nonce: "0x5",
            gasPrice: "0x0",
            gasLimit: "0x520ca",
            to: "0xdef1c0ded9bec7f1a1670819833240f027b25eff",
            value: "0x9cacb762984000",
            data: _transactionBase64ToData(

              "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"
            ),
            signOnly: false,
            signedTransaction: nil
          ),
          chainId: BraveWallet.MainnetChainId,
          maxPriorityFeePerGas: "0x77359400",
          maxFeePerGas: "0x39bdf3b000",
          gasEstimation: nil
        )
      ),
      txStatus: .confirmed,
      txType: .other,
      txParams: [],
      txArgs: [],
      createdTime: Date(timeIntervalSince1970: 1_636_399_671),  // Monday, November 8, 2021 7:27:51 PM
      submittedTime: Date(timeIntervalSince1970: 1_636_399_673),  // Monday, November 8, 2021 7:27:53 PM
      confirmedTime: Date(timeIntervalSince1970: 1_636_402_508),  // Monday, November 8, 2021 8:15:08 PM
      originInfo: .init(),
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: "0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      isRetriable: false,
      swapInfo: .init(
        from: .eth,
        fromChainId: BraveWallet.MainnetChainId,
        fromAsset: BraveWallet.ethSwapAddress,
        fromAmount: "0x1b6951ef585a000",
        to: .eth,
        toChainId: BraveWallet.MainnetChainId,
        toAsset: "0xad6d458402f60fd3bd25163575031acdce07538d",
        toAmount: "0x5c6f2d76e910358b",
        receiver: "0x099140a37d5e1da04ce05294594d27a90a4cbc06",
        provider: "zeroex"
      )
    )
  }
  /// Approved Unlimited DAI
  static var previewConfirmedERC20Approve: BraveWallet.TransactionInfo {
    BraveWallet.TransactionInfo(
      id: "19819c05-612a-47c5-84b0-e95045d15b37",
      fromAddress: BraveWallet.AccountInfo.previewAccount.accountId.address,
      from: BraveWallet.AccountInfo.previewAccount.accountId,
      txHash: "0x46d0ecf2ec9829d451154767c98ae372413bac809c25b16d1946aba100663e4b",
      txDataUnion: .init(
        ethTxData1559: .init(
          baseData: .init(
            nonce: "0x5",
            gasPrice: "0x0",
            gasLimit: "0x520ca",
            to: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
            value: "0x0",
            data: _transactionBase64ToData(
              "CV6nswAAAAAAAAAAAAAAAOWSQnoK7Okt4+3uHxjgFXwFhhVk//////////////////////////////////////////8="
            ),
            signOnly: false,
            signedTransaction: nil
          ),
          chainId: BraveWallet.MainnetChainId,
          maxPriorityFeePerGas: "0x77359400",
          maxFeePerGas: "0x39bdf3b000",
          gasEstimation: nil
        )
      ),
      txStatus: .confirmed,
      txType: .erc20Approve,
      txParams: ["address", "uint256"],
      txArgs: [
        "0xe592427a0aece92de3edee1f18e0157c05861564Z",
        "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
      ],
      createdTime: Date(timeIntervalSince1970: 1_636_399_671),  // Monday, November 8, 2021 7:27:51 PM
      submittedTime: Date(timeIntervalSince1970: 1_636_399_673),  // Monday, November 8, 2021 7:27:53 PM
      confirmedTime: Date(timeIntervalSince1970: 1_636_402_508),  // Monday, November 8, 2021 8:15:08 PM
      originInfo: .init(),
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
      isRetriable: false,
      swapInfo: nil
    )
  }
  /// Sent `mockERC721NFTToken` NFT
  static var previewERC721Send: BraveWallet.TransactionInfo {
    BraveWallet.TransactionInfo(
      id: "81111c05-612a-47c5-84b0-e95045d15b37",
      fromAddress: BraveWallet.AccountInfo.previewAccount.accountId.address,
      from: BraveWallet.AccountInfo.previewAccount.accountId,
      txHash: "0x46d0ecf2ec9829d451154767c98ae372413bac809c25b16d1946aba100663e4b",
      txDataUnion: .init(
        ethTxData1559: .init(
          baseData: .init(
            nonce: "0x5",
            gasPrice: "0xa06907542",
            gasLimit: "0x12e61",
            to: BraveWallet.BlockchainToken.mockERC721NFTToken.contractAddress,
            value: "0x0",
            data: _transactionBase64ToData(
              "CV6nswAAAAAAAAAAAAAAAOWSQnoK7Okt4+3uHxjgFXwFhhVk//////////////////////////////////////////8="
            ),
            signOnly: false,
            signedTransaction: nil
          ),
          chainId: BraveWallet.MainnetChainId,
          maxPriorityFeePerGas: "",
          maxFeePerGas: "",
          gasEstimation: nil
        )
      ),
      txStatus: .confirmed,
      txType: .erc721SafeTransferFrom,
      txParams: ["address", "address", "uint256"],
      txArgs: [
        "0x35dcec532e809a3daa04ed3fd958586f7bac9191",  // owner
        "0x3f2116ef98fcab1a9c3c2d8988e0064ab59acfca",  // to address
        BraveWallet.BlockchainToken.mockERC721NFTToken.tokenId,  // token id
      ],
      createdTime: Date(timeIntervalSince1970: 1_636_399_671),  // Monday, November 8, 2021 7:27:51 PM
      submittedTime: Date(timeIntervalSince1970: 1_636_399_673),  // Monday, November 8, 2021 7:27:53 PM
      confirmedTime: Date(timeIntervalSince1970: 1_636_402_508),  // Monday, November 8, 2021 8:15:08 PM
      originInfo: .init(),
      chainId: BraveWallet.MainnetChainId,
      effectiveRecipient: "0x3f2116ef98fcab1a9c3c2d8988e0064ab59acfca",
      isRetriable: false,
      swapInfo: nil
    )
  }
  /// Solana System Transfer
  static var previewConfirmedSolSystemTransfer: BraveWallet.TransactionInfo {
    BraveWallet.TransactionInfo(
      id: "3d3c7715-f5f2-4f70-ab97-7fb8d3b2a3cd",
      fromAddress: BraveWallet.AccountInfo.mockSolAccount.accountId.address,
      from: BraveWallet.AccountInfo.mockSolAccount.accountId,
      txHash:
        "2rbyfcSQ9xCem4wtpjMYD4u6PdKE9YcBurCHDgkMcAaBMh8CirQvuLYtj8AyaYu62ekwWKM1UDZ2VLRB4uN96Fcu",
      txDataUnion: .init(
        solanaTxData: .init(
          recentBlockhash: "",
          lastValidBlockHeight: 0,
          feePayer: BraveWallet.AccountInfo.mockSolAccount.accountId.address,
          toWalletAddress: "FoVyVfWMwoK7QgS4fcULpPSdLEp2PB5aj5ATs8VhPEv2",
          tokenAddress: "",
          lamports: UInt64(100_000_000),
          amount: UInt64(0),
          txType: .solanaSystemTransfer,
          instructions: [.init()],
          version: .legacy,
          messageHeader: .init(),
          staticAccountKeys: [],
          addressTableLookups: [],
          send: nil,
          signTransactionParam: nil,
          feeEstimation: nil
        )
      ),
      txStatus: .confirmed,
      txType: .solanaSystemTransfer,
      txParams: [],
      txArgs: [],
      createdTime: Date(timeIntervalSince1970: 1_667_854_800),  // Monday, November 7, 2022 9:00:00 PM GMT
      submittedTime: Date(timeIntervalSince1970: 1_667_854_810),  // Monday, November 7, 2022 9:00:10 PM GMT
      confirmedTime: Date(timeIntervalSince1970: 1_667_854_820),  // Monday, November 7, 2022 9:00:20 PM GMT
      originInfo: .init(),
      chainId: BraveWallet.SolanaMainnet,
      effectiveRecipient: nil,  // Currently only available for ETH and FIL
      isRetriable: false,
      swapInfo: nil
    )
  }
  /// Solana Token Transfer
  static var previewConfirmedSolTokenTransfer: BraveWallet.TransactionInfo {
    BraveWallet.TransactionInfo(
      id: "12345675-f5f2-4f70-ab97-7fb8d3b2a3cd",
      fromAddress: BraveWallet.AccountInfo.mockSolAccount.accountId.address,
      from: BraveWallet.AccountInfo.mockSolAccount.accountId,
      txHash: "",
      txDataUnion: .init(
        solanaTxData: .init(
          recentBlockhash: "",
          lastValidBlockHeight: 0,
          feePayer: BraveWallet.AccountInfo.mockSolAccount.accountId.address,
          toWalletAddress: "FoVyVfWMwoK7QgS4fcULpPSdLEp2PB5aj5ATs8VhPEv2",
          tokenAddress: "0x1111111111222222222233333333334444444444",
          lamports: UInt64(0),
          amount: UInt64(100_000_000),
          txType: .solanaSplTokenTransfer,
          instructions: [.init()],
          version: .legacy,
          messageHeader: .init(),
          staticAccountKeys: [],
          addressTableLookups: [],
          send: nil,
          signTransactionParam: nil,
          feeEstimation: nil
        )
      ),
      txStatus: .confirmed,
      txType: .solanaSplTokenTransfer,
      txParams: [],
      txArgs: [],
      createdTime: Date(timeIntervalSince1970: 1_636_399_671),  // Monday, November 8, 2021 7:27:51 PM
      submittedTime: Date(timeIntervalSince1970: 1_636_399_673),  // Monday, November 8, 2021 7:27:53 PM
      confirmedTime: Date(timeIntervalSince1970: 1_636_402_508),  // Monday, November 8, 2021 8:15:08 PM
      originInfo: .init(),
      chainId: BraveWallet.SolanaMainnet,
      effectiveRecipient: nil,  // Currently only available for ETH and FIL
      isRetriable: false,
      swapInfo: nil
    )
  }
  /// Filecoin Unapproved Send
  static let mockFilUnapprovedSend = BraveWallet.TransactionInfo(
    id: UUID().uuidString,
    fromAddress: "t165quq7gkjh6ebshr7qi2ud7vycel4m7x6dvfvgb",
    from: BraveWallet.AccountInfo.mockFilAccount.accountId,
    txHash: "",
    txDataUnion:
      .init(
        filTxData:
          .init(
            nonce: "",
            gasPremium: "100911",
            gasFeeCap: "101965",
            gasLimit: "1527953",
            maxFee: "0",
            to: "t1xqhfiydm2yq6augugonr4zpdllh77iw53aexztb",
            value: "1000000000000000000"
          )
      ),
    txStatus: .unapproved,
    txType: .other,
    txParams: [],
    txArgs: [],
    createdTime: Date(timeIntervalSince1970: 1_636_399_671),  // Monday, November 8, 2021 7:27:51 PM
    submittedTime: Date(timeIntervalSince1970: 1_636_399_673),  // Monday, November 8, 2021 7:27:53 PM
    confirmedTime: Date(timeIntervalSince1970: 1_636_402_508),  // Monday, November 8, 2021 8:15:08 PM
    originInfo: nil,
    chainId: BraveWallet.FilecoinMainnet,
    effectiveRecipient: nil,
    isRetriable: false,
    swapInfo: nil
  )
  /// Bitcoin Unapproved Send
  static let mockBTCUnapprovedSend = BraveWallet.TransactionInfo(
    id: "9",
    fromAddress: BraveWallet.AccountInfo.mockBtcAccount.address,
    from: BraveWallet.AccountInfo.mockBtcAccount.accountId,
    txHash: "",
    txDataUnion:
      .init(
        btcTxData:
          .init(
            to: "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4",
            amount: 5000,
            sendingMaxAmount: false,
            fee: 2544,
            inputs: [],
            outputs: []
          )
      ),
    txStatus: .unapproved,
    txType: .other,
    txParams: [],
    txArgs: [],
    createdTime: Date(timeIntervalSince1970: 1_715_280_785),  // Thursday, May 9, 2024 6:53:05 PM
    submittedTime: Date(timeIntervalSince1970: 1_715_280_790),  // Thursday, May 9, 2024 6:53:10 PM
    confirmedTime: Date(timeIntervalSince1970: 1_715_281_790),  // Thursday, May 9, 2024 7:09:50 PM
    originInfo: nil,
    chainId: BraveWallet.BitcoinMainnet,
    effectiveRecipient: nil,
    isRetriable: false,
    swapInfo: nil
  )
  static private func _transactionBase64ToData(_ base64String: String) -> [NSNumber] {
    guard let data = Data(base64Encoded: base64String) else { return [] }
    return Array(data).map(NSNumber.init(value:))
  }
}

extension BraveWallet.SignMessageRequest {
  static var previewRequest: BraveWallet.SignMessageRequest {
    .init(
      originInfo: .init(originSpec: "https://app.uniswap.org", eTldPlusOne: "uniswap.org"),
      id: 1,
      accountId: BraveWallet.AccountInfo.previewAccount.accountId,
      signData: .init(
        ethSignTypedData: .init(
          addressParam: "address",
          messageJson: "To avoid digital cat burglars, sign below to authenticate with CryptoKitties.",
          domainJson: "{}",
          typesJson: "{}",
          primaryType: "primaryType",
          chainId: nil,
          domainHash: [],
          primaryHash: [],
          meta: nil
        )
      ),
      coin: .eth,
      chainId: BraveWallet.MainnetChainId
    )
  }
}

extension ParsedTransaction {

  static var previewConfirmedSend = previewParsedTransaction(from: .previewConfirmedSend)
  static var previewConfirmedSwap = previewParsedTransaction(from: .previewConfirmedSwap)
  static var previewConfirmedERC20Approve = previewParsedTransaction(
    from: .previewConfirmedERC20Approve
  )

  static func previewParsedTransaction(from txInfo: BraveWallet.TransactionInfo) -> Self? {
    TransactionParser.parseTransaction(
      transaction: txInfo,
      allNetworks: [.mockMainnet],
      accountInfos: [.previewAccount],
      userAssets: [.previewToken, .previewDaiToken],
      allTokens: [],
      assetRatios: [BraveWallet.BlockchainToken.previewToken.assetRatioId.lowercased(): 1],
      nftMetadata: [:],
      solEstimatedTxFee: nil,
      currencyFormatter: .usdCurrencyFormatter
    )
  }
}

extension BraveWallet.CoinMarket {
  static var mockCoinMarketBitcoin: BraveWallet.CoinMarket {
    .init(
      id: "bitcoin",
      symbol: "btc",
      name: "Bitcoin",
      image: "https://assets.cgproxy.brave.com/coins/images/1/large/bitcoin.png?1547033579",
      marketCap: 547_558_353_670,
      marketCapRank: 1,
      currentPrice: 28324,
      priceChange24h: 163.96,
      priceChangePercentage24h: 0.58225,
      totalVolume: 30_825_602_847
    )
  }
  static var mockCoinMarketEth: BraveWallet.CoinMarket {
    .init(
      id: "ethereum",
      symbol: "eth",
      name: "Ethereum",
      image: "https://assets.cgproxy.brave.com/coins/images/279/large/ethereum.png?1595348880",
      marketCap: 223_719_056_533,
      marketCapRank: 2,
      currentPrice: 1860.57,
      priceChange24h: -4.2550480604149925,
      priceChangePercentage24h: -0.22817,
      totalVolume: 15_998_007_227
    )
  }
}

extension BraveWallet.SwapFees {
  static var mockJupiterFees: BraveWallet.SwapFees {
    BraveWallet.SwapFees(
      feeParam: "85",
      feePct: "0.85",
      discountPct: "0",
      effectiveFeePct: "0.85",
      discountCode: .none
    )
  }

  static var mockEthFees: BraveWallet.SwapFees {
    BraveWallet.SwapFees(
      feeParam: "0.00875",
      feePct: "0.875",
      discountPct: "0",
      effectiveFeePct: "0.875",
      discountCode: .none
    )
  }
}

extension BraveWallet.ZeroExQuote {
  /// Price quote for 1 ETH to BAT (sell amount entered)
  static let mockOneETHToBATQuote: BraveWallet.ZeroExQuote = .init(
    buyAmount: "10137516188895530252258",
    buyToken: "0x0d8775f648430679a709e98d2b0cb6250d2887ef",  // BAT
    fees: .init(
      zeroExFee: .init(
        amount: "15286142457313100889",
        token: "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        type: "volume"
      )
    ),
    gas: "375000",
    gasPrice: "46500000000",
    liquidityAvailable: true,
    minBuyAmount: "99032421",
    route: .init(
      fills: [.init(
        from: "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
        to: "0xdac17f958d2ee523a2206206994597c13d831ec7",
        source: "SolidlyV3",
        proportionBps: "10000"
      )]
    ),
    sellAmount: "1000000000000000000",  // 1 ETH
    sellToken: "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",  // ETH
    totalNetworkFee: "2034668056550000",
    allowanceTarget: "0x0000000000000000000000000000000000000000"
  )
}

extension BraveWallet.LiFiQuote {
  /// Price quote for 1 ETH to USDC (sell amount entered)
  static let mockOneETHtoUSDCQuote: BraveWallet.LiFiQuote = .init(
    routes: [
      .init(
        id: "route.1",
        uniqueId: "allbridge",
        from: .previewToken,
        fromAmount: "1000000000000000000",
        fromAddress: BraveWallet.AccountInfo.mockEthAccount.address,
        to: .mockUSDCToken,
        toAmount: "3537722948",
        toAmountMin: "",
        toAddress: BraveWallet.AccountInfo.mockEthAccount.address,
        steps: [
          .init(
            id: "step.1",
            type: .liFi,
            tool: "1inch",
            toolDetails: .init(key: "", name: "", logo: ""),
            action: .init(),
            estimate: .init(
              tool: "1inch",
              fromAmount: "1000000000000000000",
              toAmount: "3537722948",
              toAmountMin: "3520034333",
              approvalAddress: "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE",
              executionDuration: "30",
              feeCosts: nil,
              gasCosts: [
                .init(
                  type: "SEND",
                  estimate: "386779",
                  limit: "729779",
                  amount: "15403648617462374",
                  token: .previewToken
                )
              ]
            ),
            integrator: nil,
            includedSteps: nil
          )
        ],
        tags: []
      )
    ]
  )
}

#endif
