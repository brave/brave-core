import XCTest
import BraveCore
@testable import BraveWallet

class BraveWalletExtensionsTests: XCTestCase {
  
  func testIsNativeAsset() {
    let ethCoin: BraveWallet.BlockchainToken = .init(contractAddress: "", name: "Eth", logo: "", isErc20: false, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "Eth", decimals: 18, visible: true, tokenId: "", coingeckoId: "", chainId: "", coin: .eth)
    let ethToken: BraveWallet.BlockchainToken = .init(contractAddress: "mock-eth-contract-address", name: "Eth Token", logo: "", isErc20: true, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "ETHTOKEN", decimals: 18, visible: false, tokenId: "", coingeckoId: "", chainId: "", coin: .eth)
    let ethNetwork = BraveWallet.NetworkInfo.mockMainnet
    
    XCTAssertTrue(ethNetwork.isNativeAsset(ethCoin))
    XCTAssertFalse(ethNetwork.isNativeAsset(ethToken))
    
    let solCoin: BraveWallet.BlockchainToken = .init(contractAddress: "", name: "Sol", logo: "", isErc20: false, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "SOL", decimals: 9, visible: true, tokenId: "", coingeckoId: "", chainId: "", coin: .sol)
    let solToken: BraveWallet.BlockchainToken = .init(contractAddress: "mock-sol-contract-address", name: "Sol Token", logo: "", isErc20: true, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "SOLTOKEN", decimals: 9, visible: false, tokenId: "", coingeckoId: "", chainId: "", coin: .sol)
    let solNetwork = BraveWallet.NetworkInfo.mockSolana
    
    XCTAssertTrue(solNetwork.isNativeAsset(solCoin))
    XCTAssertFalse(solNetwork.isNativeAsset(solToken))
  }
}
