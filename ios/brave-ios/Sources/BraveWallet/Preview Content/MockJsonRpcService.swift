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
  private var networks: [BraveWallet.NetworkInfo] = [.mockMainnet, .mockGoerli, .mockSepolia, .mockPolygon, .mockCelo]
  private var networkURL: URL?
  private var observers: NSHashTable<BraveWalletJsonRpcServiceObserver> = .weakObjects()
  
  func chainId(forOrigin coin: BraveWallet.CoinType, origin: URLOrigin, completion: @escaping (String) -> Void) {
    completion(chainId)
  }
  
  func blockTrackerUrl(_ completion: @escaping (String) -> Void) {
    completion(networks.first(where: { $0.chainId == self.chainId })?.blockExplorerUrls.first ?? "")
  }
  
  func networkUrl(_ coin: BraveWallet.CoinType, completion: @escaping (String) -> Void) {
    completion(networkURL?.absoluteString ?? "")
  }
  
  func network(_ coin: BraveWallet.CoinType, origin: URLOrigin?, completion: @escaping (BraveWallet.NetworkInfo) -> Void) {
    completion(networks.first(where: { $0.chainId == self.chainId }) ?? .init())
  }
  
  func balance(_ address: String, coin: BraveWallet.CoinType, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    // return fake sufficient ETH balance `0x13e25e19dc20ba7` is about 0.0896 ETH
    completion("0x13e25e19dc20ba7", .success, "")
  }
  
  func erc20TokenBalance(_ contract: String, address: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("10", .success, "")
  }
  
  func erc20TokenBalances(_ contracts: [String], address: String, chainId: String, completion: @escaping ([BraveWallet.ERC20BalanceResult], BraveWallet.ProviderError, String) -> Void) {
    completion([.init(contractAddress: "", balance: "10")], .success, "")
  }
  
  func request(_ chainId: String, jsonPayload: String, autoRetryOnNetworkChange: Bool, id: MojoBase.Value, coin: BraveWallet.CoinType, completion: @escaping (MojoBase.Value, MojoBase.Value, Bool, String, Bool) -> Void) {
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
  
  func setNetwork(_ chainId: String, coin: BraveWallet.CoinType, origin: URLOrigin?, completion: @escaping (Bool) -> Void) {
    self.chainId = chainId
    completion(true)
  }
  
  func erc20TokenAllowance(_ contract: String, ownerAddress: String, spenderAddress: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .disconnected, "Error Message")
  }
  
  func enableEnsOffchainLookup() {
  }
  
  func ensGetEthAddr(_ domain: String, completion: @escaping (String, Bool, BraveWallet.ProviderError, String) -> Void) {
    completion("", false, .internalError, "")
  }
  
  func unstoppableDomainsGetWalletAddr(_ domain: String, token: BraveWallet.BlockchainToken?, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .unknown, "Error Message")
  }
  
  func unstoppableDomainsResolveDns(_ domain: String, completion: @escaping (URL?, BraveWallet.ProviderError, String) -> Void) {
    completion(nil, .internalError, "Error message")
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
  
  func removeChain(_ chainId: String, coin: BraveWallet.CoinType, completion: @escaping (Bool) -> Void) {
    if let historyIndex = networks.firstIndex(where: { $0.chainId == chainId }) {
      networks.remove(at: historyIndex)
      completion(true)
    } else {
      completion(false)
    }
  }
  
  func addChain(_ chain: BraveWallet.NetworkInfo, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    networks.append(chain)
    completion("", .success, "")
  }
  
  func addHiddenNetwork(_ coin: BraveWallet.CoinType, chainId: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func removeHiddenNetwork(_ coin: BraveWallet.CoinType, chainId: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func addEthereumChain(forOrigin chain: BraveWallet.NetworkInfo, origin: URLOrigin, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .chainDisconnected, "Error Message")
  }
  
  func addEthereumChainRequestCompleted(_ chainId: String, approved: Bool) {
  }
  
  func notifySwitchChainRequestProcessed(_ requestId: String, approved: Bool) {
  }
  
  func setCustomNetworkForTesting(_ chainId: String, coin: BraveWallet.CoinType, providerUrl: URL) {
  }
  
  func solanaBalance(_ pubkey: String, chainId: String, completion: @escaping (UInt64, BraveWallet.SolanaProviderError, String) -> Void) {
    completion(0, .internalError, "Error Message")
  }
  
  func splTokenAccountBalance(_ walletAddress: String, tokenMintAddress: String, chainId: String, completion: @escaping (String, UInt8, String, BraveWallet.SolanaProviderError, String) -> Void) {
    completion("", 0, "", .internalError, "Error Message")
  }

  func splTokenBalances(_ pubkey: String, chainId: String, completion: @escaping ([BraveWallet.SPLTokenAmount], BraveWallet.SolanaProviderError, String) -> Void) {
    completion([], .internalError, "Error Message")
  }
  
  func erc1155TokenBalance(_ contractAddress: String, tokenId: String, accountAddress: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .internalError, "")
  }
  
  func erc721Metadata(_ contract: String, tokenId: String, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .internalError, "")
  }
  
  func solTokenMetadata(_ chainId: String, tokenMintAddress: String, completion: @escaping (String, String, BraveWallet.SolanaProviderError, String) -> Void) {
    completion("", "", .internalError, "")
  }
  
  func erc721Metadata(_ contract: String, tokenId: String, chainId: String, completion: @escaping (String, String, BraveWallet.ProviderError, String) -> Void) {
    completion("", "", .internalError, "")
  }
  
  func erc1155Metadata(_ contract: String, tokenId: String, chainId: String, completion: @escaping (String, String, BraveWallet.ProviderError, String) -> Void) {
    completion("", "", .internalError, "")
  }
  
  func hiddenNetworks(_ coin: BraveWallet.CoinType, completion: @escaping ([String]) -> Void) {
    completion([])
  }
  
  func customNetworks(_ coin: BraveWallet.CoinType, completion: @escaping ([String]) -> Void) {
    completion([])
  }
  
  func knownNetworks(_ coin: BraveWallet.CoinType, completion: @escaping ([String]) -> Void) {
    completion([])
  }
  
  func snsGetSolAddr(_ domain: String, completion: @escaping (String, BraveWallet.SolanaProviderError, String) -> Void) {
    completion("", .internalError, "Error Message")
  }
  
  func snsResolveHost(_ domain: String, completion: @escaping (URL?, BraveWallet.SolanaProviderError, String) -> Void) {
    completion(nil, .internalError, "Error Message")
  }

  func unstoppableDomainsResolveMethod(_ completion: @escaping (BraveWallet.ResolveMethod) -> Void) {
    completion(.ask)
  }

  func ensResolveMethod(_ completion: @escaping (BraveWallet.ResolveMethod) -> Void) {
    completion(.ask)
  }

  func ensOffchainLookupResolveMethod(_ completion: @escaping (BraveWallet.ResolveMethod) -> Void) {
    completion(.ask)
  }

  func snsResolveMethod(_ completion: @escaping (BraveWallet.ResolveMethod) -> Void) {
    completion(.ask)
  }

  func setUnstoppableDomainsResolveMethod(_ method: BraveWallet.ResolveMethod) {
  }

  func setEnsResolveMethod(_ method: BraveWallet.ResolveMethod) {
  }

  func setEnsOffchainLookupResolveMethod(_ method: BraveWallet.ResolveMethod) {
  }

  func setSnsResolveMethod(_ method: BraveWallet.ResolveMethod) {
  }
  
  func code(_ address: String, coin: BraveWallet.CoinType, chainId: String, completion: @escaping (String, BraveWallet.ProviderError, String) -> Void) {
    completion("", .internalError, "Error Message")
  }
  
  func ensGetContentHash(_ domain: String, completion: @escaping ([NSNumber], Bool, BraveWallet.ProviderError, String) -> Void) {
    completion([], false, .internalError, "Error Message")
  }
  
  func defaultChainId(_ coin: BraveWallet.CoinType, completion: @escaping (String) -> Void) {
    switch coin {
    case .eth:
      completion(BraveWallet.MainnetChainId)
    case .sol:
      completion(BraveWallet.SolanaMainnet)
    case .fil:
      completion(BraveWallet.FilecoinMainnet)
    case .btc:
      fallthrough
    default:
      completion("")
    }
  }
  
  func networkUrl(_ coin: BraveWallet.CoinType, origin: URLOrigin?, completion: @escaping (String) -> Void) {
    completion("")
  }
  
  func isSolanaBlockhashValid(_ chainId: String, blockhash: String, commitment: String?, completion: @escaping (Bool, BraveWallet.SolanaProviderError, String) -> Void) {
    completion(true, .success, "")
  }
  
  func ethTokenInfo(_ contractAddress: String, chainId: String, completion: @escaping (BraveWallet.BlockchainToken?, BraveWallet.ProviderError, String) -> Void) {
    completion(nil, .internalError, "Error Message")
  }
  
  func ankrGetAccountBalances(_ accountAddress: String, chainIds: [String], completion: @escaping ([BraveWallet.AnkrAssetBalance], BraveWallet.ProviderError, String) -> Void) {
    completion([], .internalError, "Error Message")
  }
}

extension BraveWallet.NetworkInfo {
  static let mockMainnet: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.MainnetChainId,
    chainName: "Mainnet",
    blockExplorerUrls: ["https://etherscan.io"],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://rpc.mockchain.com")!],
    symbol: "ETH",
    symbolName: "Ethereum",
    decimals: 18,
    coin: .eth,
    supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:)),
    isEip1559: true
  )
  static let mockGoerli: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.GoerliChainId,
    chainName: "Goerli",
    blockExplorerUrls: ["https://goerli.etherscan.io"],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://rpc.mockchain.com")!],
    symbol: "ETH",
    symbolName: "Ethereum",
    decimals: 18,
    coin: .eth,
    supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:)),
    isEip1559: true
  )
  static let mockSepolia: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.SepoliaChainId,
    chainName: "Sepolia",
    blockExplorerUrls: ["https://sepolia.etherscan.io"],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://rpc.mockchain.com")!],
    symbol: "ETH",
    symbolName: "Ethereum",
    decimals: 18,
    coin: .eth,
    supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:)),
    isEip1559: true
  )
  static let mockPolygon: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.PolygonMainnetChainId,
    chainName: "Polygon Mainnet",
    blockExplorerUrls: [""],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://rpc.mockchain.com")!],
    symbol: "MATIC",
    symbolName: "MATIC",
    decimals: 18,
    coin: .eth,
    supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:)),
    isEip1559: true
  )
  static let mockCelo: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.CeloMainnetChainId,
    chainName: "Celo Mainnet",
    blockExplorerUrls: [""],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://rpc.mockchain.com")!],
    symbol: "CELO",
    symbolName: "CELO",
    decimals: 18,
    coin: .eth,
    supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:)),
    isEip1559: false
  )
  static let mockSolana: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.SolanaMainnet,
    chainName: "Solana Mainnet",
    blockExplorerUrls: [""],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://rpc.mockchain.com")!],
    symbol: "SOL",
    symbolName: "Solana",
    decimals: 9,
    coin: .sol,
    supportedKeyrings: [BraveWallet.KeyringId.solana.rawValue].map(NSNumber.init(value:)),
    isEip1559: false
  )
  static let mockSolanaTestnet: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.SolanaTestnet,
    chainName: "Solana Testnet",
    blockExplorerUrls: [""],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://rpc.mockchain.com")!],
    symbol: "SOL",
    symbolName: "Solana",
    decimals: 9,
    coin: .sol,
    supportedKeyrings: [BraveWallet.KeyringId.solana.rawValue].map(NSNumber.init(value:)),
    isEip1559: false
  )
  static let mockFilecoinMainnet: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.FilecoinMainnet,
    chainName: "Filecoin Mainnet",
    blockExplorerUrls: [""],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://rpc.ankr.com/filecoin")!],
    symbol: "FIL",
    symbolName: "Filecoin",
    decimals: 18,
    coin: .fil,
    supportedKeyrings: [BraveWallet.KeyringId.filecoin.rawValue].map(NSNumber.init(value:)),
    isEip1559: true
  )
  static let mockFilecoinTestnet: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.FilecoinTestnet,
    chainName: "Filecoin Testnet",
    blockExplorerUrls: [""],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://rpc.ankr.com/filecoin_testnet")!],
    symbol: "FIL",
    symbolName: "Filecoin",
    decimals: 18,
    coin: .fil,
    supportedKeyrings: [BraveWallet.KeyringId.filecoinTestnet.rawValue].map(NSNumber.init(value:)),
    isEip1559: true
  )
}

#endif
