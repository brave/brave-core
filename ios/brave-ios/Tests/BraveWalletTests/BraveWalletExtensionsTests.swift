import BraveCore
import XCTest

@testable import BraveWallet

class BraveWalletExtensionsTests: XCTestCase {

  func testIsNativeAsset() {
    let ethCoin: BraveWallet.BlockchainToken = .init(
      contractAddress: "",
      name: "Eth",
      logo: "",
      isCompressed: false,
      isErc20: false,
      isErc721: false,
      isErc1155: false,
      splTokenProgram: .unsupported,
      isNft: false,
      isSpam: false,
      symbol: "Eth",
      decimals: 18,
      visible: true,
      tokenId: "",
      coingeckoId: "",
      chainId: "",
      coin: .eth,
      zcashTokenType: .none
    )
    let ethToken: BraveWallet.BlockchainToken = .init(
      contractAddress: "mock-eth-contract-address",
      name: "Eth Token",
      logo: "",
      isCompressed: false,
      isErc20: true,
      isErc721: false,
      isErc1155: false,
      splTokenProgram: .unsupported,
      isNft: false,
      isSpam: false,
      symbol: "ETHTOKEN",
      decimals: 18,
      visible: false,
      tokenId: "",
      coingeckoId: "",
      chainId: "",
      coin: .eth,
      zcashTokenType: .none
    )
    let ethNetwork = BraveWallet.NetworkInfo.mockMainnet

    XCTAssertTrue(ethNetwork.isNativeAsset(ethCoin))
    XCTAssertFalse(ethNetwork.isNativeAsset(ethToken))

    let solCoin: BraveWallet.BlockchainToken = .init(
      contractAddress: "",
      name: "Sol",
      logo: "",
      isCompressed: false,
      isErc20: false,
      isErc721: false,
      isErc1155: false,
      splTokenProgram: .unknown,
      isNft: false,
      isSpam: false,
      symbol: "SOL",
      decimals: 9,
      visible: true,
      tokenId: "",
      coingeckoId: "",
      chainId: "",
      coin: .sol,
      zcashTokenType: .none
    )
    let solToken: BraveWallet.BlockchainToken = .init(
      contractAddress: "mock-sol-contract-address",
      name: "Sol Token",
      logo: "",
      isCompressed: false,
      isErc20: true,
      isErc721: false,
      isErc1155: false,
      splTokenProgram: .unknown,
      isNft: false,
      isSpam: false,
      symbol: "SOLTOKEN",
      decimals: 9,
      visible: false,
      tokenId: "",
      coingeckoId: "",
      chainId: "",
      coin: .sol,
      zcashTokenType: .none
    )
    let solNetwork = BraveWallet.NetworkInfo.mockSolana

    XCTAssertTrue(solNetwork.isNativeAsset(solCoin))
    XCTAssertFalse(solNetwork.isNativeAsset(solToken))
  }

  func testIsGrantingERC721SetApprovalForAll() {
    let spenderAddress = "0x0987654321098765432109876543210987654321"
    func transaction(
      txType: BraveWallet.TransactionType,
      txArgs: [String]
    ) -> BraveWallet.TransactionInfo {
      BraveWallet.TransactionInfo(
        id: "1",
        from: BraveWallet.AccountInfo.previewAccount.accountId,
        txHash: "",
        txDataUnion: .init(
          ethTxData1559: .init(
            baseData: .init(
              chainId: BraveWallet.MainnetChainId,
              nonce: "1",
              gasPrice: "0x0",
              gasLimit: "0x5208",
              to: spenderAddress,
              value: "0x0",
              data: []
            ),
            maxPriorityFeePerGas: "0x59672ead",
            maxFeePerGas: "0x59672eb6"
          )
        ),
        txStatus: .unapproved,
        txType: txType,
        txParams: [],
        txArgs: txArgs,
        createdTime: Date(),
        submittedTime: Date(),
        confirmedTime: Date(),
        dappOriginInfo: nil,
        chainId: BraveWallet.MainnetChainId,
        effectiveRecipient: nil,
        isRetriable: false,
        swapInfo: nil,
        swapInfoDeprecated: nil
      )
    }

    XCTAssertTrue(
      transaction(
        txType: .erc721SetApprovalForAll,
        txArgs: [spenderAddress, "0x1"]
      ).isGrantingERC721SetApprovalForAll
    )
    XCTAssertTrue(
      transaction(
        txType: .erc721SetApprovalForAll,
        txArgs: [spenderAddress, "0X1"]
      ).isGrantingERC721SetApprovalForAll
    )
    // Revoking approval must not be treated as granting.
    XCTAssertFalse(
      transaction(
        txType: .erc721SetApprovalForAll,
        txArgs: [spenderAddress, "0x0"]
      ).isGrantingERC721SetApprovalForAll
    )
    XCTAssertFalse(
      transaction(
        txType: .erc721SetApprovalForAll,
        txArgs: [spenderAddress]
      ).isGrantingERC721SetApprovalForAll
    )
    XCTAssertFalse(
      transaction(
        txType: .erc20Approve,
        txArgs: [spenderAddress, "0x1"]
      ).isGrantingERC721SetApprovalForAll
    )
  }
}
