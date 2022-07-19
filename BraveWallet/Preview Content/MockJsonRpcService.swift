/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveCore

#if DEBUG

/// A test eth json controller which can be passed to a ``NetworkStore`` that implements some basic
/// functionality for the use of SwiftUI Previews.
///
/// - note: Do not use this directly, use ``NetworkStore.previewStore``
class MockJsonRpcService: BraveWalletJsonRpcService {
  
  private var chainId: String = BraveWallet.MainnetChainId
  private var networks: [BraveWallet.NetworkInfo] = [.mockMainnet, .mockRinkeby, .mockRopsten]
  private var networkURL: URL?
  private var observers: NSHashTable<BraveWalletJsonRpcServiceObserver> = .weakObjects()
  
  func chainId(_ coin: BraveWallet.CoinType, completion: @escaping (String) -> Void) {
    completion(chainId)
  }
  
  func blockTrackerUrl(_ completion: @escaping (String) -> Void) {
    completion(networks.first(where: { $0.chainId == self.chainId })?.blockExplorerUrls.first ?? "")
  }
  
  func networkUrl(_ coin: BraveWallet.CoinType, completion: @escaping (String) -> Void) {
    completion(networkURL?.absoluteString ?? "")
  }
  
  func network(_ coin: BraveWallet.CoinType, completion: @escaping (BraveWallet.NetworkInfo) -> Void) {
    completion(networks.first(where: { $0.chainId == self.chainId }) ?? .init())
  }
  
  func balance(_ address: String, coin: BraveWallet.CoinType, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    // return fake sufficient ETH balance `0x13e25e19dc20ba7` is about 0.0896 ETH
    completion("0x13e25e19dc20ba7", .success, "")
  }
  
  func erc20TokenBalance(_ contract: String, address: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("10", .success, "")
  }
  
  func request(_ jsonPayload: String, autoRetryOnNetworkChange: Bool, id: MojoBase.Value, coin: BraveWallet.CoinType, completion: @escaping (MojoBase.Value, MojoBase.Value, Bool, String, Bool) -> Void) {
    completion(.init(), .init(), true, "", false)
  }
  
  func add(_ observer: BraveWalletJsonRpcServiceObserver) {
    observers.add(observer)
  }
  
  func pendingAddChainRequests(_ completion: @escaping ([BraveWallet.AddChainRequest]) -> Void) {
    completion([])
  }
  
  func pendingChainRequests(_ completion: @escaping ([BraveWallet.NetworkInfo]) -> Void) {
    completion([])
  }
  
  func allNetworks(_ coin: BraveWallet.CoinType, completion: @escaping ([BraveWallet.NetworkInfo]) -> Void) {
    completion(networks)
  }
  
  func setNetwork(_ chainId: String, coin: BraveWallet.CoinType, completion: @escaping (Bool) -> Void) {
    self.chainId = chainId
    completion(true)
  }
  
  func erc20TokenAllowance(_ contract: String, ownerAddress: String, spenderAddress: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .disconnected, "Error Message")
  }
  
  func ensGetEthAddr(_ domain: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .unknownChain, "Error Message")
  }
  
  func unstoppableDomainsGetEthAddr(_ domain: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .unknownChain, "Error Message")
  }
  
  func erc721Owner(of contract: String, tokenId: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .unknownChain, "Error Message")
  }
  
  func erc721TokenBalance(_ contractAddress: String, tokenId: String, accountAddress: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .disconnected, "Error Message")
  }
  
  func pendingSwitchChainRequests(_ completion: @escaping ([BraveWallet.SwitchChainRequest]) -> Void) {
    completion([])
  }
  
  func removeEthereumChain(_ chainId: String, completion: @escaping (Bool) -> Void) {
    if let index = networks.firstIndex(where: { $0.chainId == chainId }) {
      networks.remove(at: index)
      completion(true)
    } else {
      completion(false)
    }
  }
  
  func addEthereumChain(_ chain: BraveWallet.NetworkInfo, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    networks.append(chain)
    completion("", .success, "")
  }
  
  func addEthereumChain(forOrigin chain: BraveWallet.NetworkInfo, origin: URLOrigin, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .chainDisconnected, "Error Message")
  }
  
  func addEthereumChainRequestCompleted(_ chainId: String, approved: Bool) {
  }
  
  func notifySwitchChainRequestProcessed(_ approved: Bool, origin: URLOrigin) {
  }
  
  func setCustomNetworkForTesting(_ chainId: String, coin: BraveWallet.CoinType, providerUrl: URL) {
  }
  
  func solanaBalance(_ pubkey: String, chainId: String, completion: @escaping (UInt64, BraveWallet.SolanaProviderError, String) -> Void) {
    completion(0, .internalError, "Error Message")
  }
  
  func splTokenAccountBalance(_ walletAddress: String, tokenMintAddress: String, chainId: String, completion: @escaping (String, UInt8, String, BraveWallet.SolanaProviderError, String) -> Void) {
    completion("", 0, "", .internalError, "Error Message")
  }
  
  func erc1155TokenBalance(_ contractAddress: String, tokenId: String, accountAddress: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .internalError, "")
  }
  
  func erc721Metadata(_ contract: String, tokenId: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .internalError, "")
  }
  
  func erc1155Metadata(_ contract: String, tokenId: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .internalError, "")
  }
  
  func hiddenNetworks(_ coin: BraveWallet.CoinType, completion: @escaping ([String]) -> Void) {
    completion([""])
  }
}

extension BraveWallet.NetworkInfo {
  static let mockMainnet: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.MainnetChainId,
    chainName: "Mainnet",
    blockExplorerUrls: ["https://etherscan.io"],
    iconUrls: [],
    rpcUrls: [],
    symbol: "ETH",
    symbolName: "Ethereum",
    decimals: 18,
    coin: .eth,
    data: .init(ethData: .init(isEip1559: true))
  )
  static let mockRinkeby: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.RinkebyChainId,
    chainName: "Rinkeby",
    blockExplorerUrls: ["https://rinkeby.etherscan.io"],
    iconUrls: [],
    rpcUrls: [],
    symbol: "ETH",
    symbolName: "Ethereum",
    decimals: 18,
    coin: .eth,
    data: .init(ethData: .init(isEip1559: true))
  )
  static let mockRopsten: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.RopstenChainId,
    chainName: "Ropsten",
    blockExplorerUrls: ["https://ropsten.etherscan.io"],
    iconUrls: [],
    rpcUrls: [],
    symbol: "ETH",
    symbolName: "Ethereum",
    decimals: 18,
    coin: .eth,
    data: .init(ethData: .init(isEip1559: true))
  )
  static let mockPolygon: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.PolygonMainnetChainId,
    chainName: "Polygon Mainnet",
    blockExplorerUrls: [""],
    iconUrls: [],
    rpcUrls: [],
    symbol: "MATIC",
    symbolName: "MATIC",
    decimals: 18,
    coin: .eth,
    data: .init(ethData: .init(isEip1559: true))
  )
  static let mockSolana: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.SolanaMainnet,
    chainName: "Solana Mainnet",
    blockExplorerUrls: [""],
    iconUrls: [],
    rpcUrls: [],
    symbol: "SOL",
    symbolName: "Solana",
    decimals: 18,
    coin: .sol,
    data: nil
  )
  static let mockSolanaTestnet: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.SolanaTestnet,
    chainName: "Solana Testnet",
    blockExplorerUrls: [""],
    iconUrls: [],
    rpcUrls: [],
    symbol: "SOL",
    symbolName: "Solana",
    decimals: 18,
    coin: .sol,
    data: nil
  )
}

#endif
