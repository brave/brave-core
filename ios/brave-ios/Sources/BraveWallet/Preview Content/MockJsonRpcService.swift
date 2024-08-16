// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

#if DEBUG

/// JsonRpcService implementation used for previews and unit tests.
/// Implements default responses for commonly used functions to avoid unimplemented crashes.
/// Override `_*` functions to implement unit test specific responses as needed.
class MockJsonRpcService: BraveWallet.TestJsonRpcService {
  static let allKnownNetworks: [BraveWallet.NetworkInfo] = [
    .mockMainnet, .mockPolygon, .mockSepolia,
    .mockSolana, .mockSolanaTestnet,
    .mockFilecoinMainnet, .mockFilecoinTestnet,
    .mockBitcoinMainnet, .mockBitcoinTestnet,
  ]

  var selectedNetworkForCoin: [BraveWallet.CoinType: BraveWallet.NetworkInfo] = [
    .eth: .mockMainnet,
    .sol: .mockSolana,
    .fil: .mockFilecoinMainnet,
    .btc: .mockBitcoinMainnet,
  ]

  var allCustomNetworks: [BraveWallet.NetworkInfo] = []
  var hiddenNetworks: [BraveWallet.NetworkInfo] = [
    .mockSepolia,
    .mockSolanaTestnet,
    .mockFilecoinTestnet,
    .mockBitcoinTestnet,
  ]

  override init() {
    super.init()
    self._addObserver = { _ in }
    self._allNetworks = { [weak self] completion in
      guard let self else {
        completion(Self.allKnownNetworks)
        return
      }
      completion(Self.allKnownNetworks + self.allCustomNetworks)
    }
    self._hiddenNetworks = { [weak self] _, completion in
      guard let self else {
        completion([])
        return
      }
      completion(self.hiddenNetworks.map(\.chainId))
    }
    self._addHiddenNetwork = { _, _, completion in
      completion(true)
    }
    self._removeHiddenNetwork = { _, _, completion in
      completion(true)
    }
    self._knownNetworks = { coin, completion in
      completion(Self.allKnownNetworks.filter({ $0.coin == coin }).map(\.chainId))
    }
    self._customNetworks = { [weak self] _, completion in
      guard let self else {
        completion([])
        return
      }
      completion(self.allCustomNetworks.map(\.chainId))
    }
    self._addChain = { [weak self] network, completion in
      guard let self else {
        completion("", .internalError, "Internal error")
        return
      }
      self.allCustomNetworks.append(network)
      completion(network.chainId, .success, "")
    }
    self._network = { [weak self] coin, _, completion in
      guard let self, let network = self.selectedNetworkForCoin[coin] else {
        assertionFailure("Selected network for \(coin.localizedTitle) not available.")
        return
      }
      completion(network)
    }
    self._chainIdForOrigin = { [weak self] coin, _, completion in
      guard let self, let network = self.selectedNetworkForCoin[coin] else {
        assertionFailure("Selected network for \(coin.localizedTitle) not available.")
        return
      }
      completion(network.chainId)
    }
    self._setNetwork = { [weak self] chainId, coin, _, completion in
      guard let self,
        let network = (Self.allKnownNetworks + self.allCustomNetworks).first(where: {
          $0.chainId == chainId
        })
      else {
        assertionFailure(
          "Set network for \(coin.localizedTitle) unavailable. Use addChain or add network to `allKnownNetworks`."
        )
        return
      }
      self.selectedNetworkForCoin[coin] = network
      completion(true)
    }
    self._balance = { _, coin, chainId, completion in
      completion("0", .success, "")
    }
    self._erc20TokenBalance = { _, _, _, completion in
      completion("0", .success, "")
    }
    self._erc20TokenBalances = { contractAddresses, _, _, completion in
      let erc20BalanceResults = contractAddresses.map { contractAddress in
        BraveWallet.ERC20BalanceResult(contractAddress: contractAddress, balance: "0")
      }
      completion(erc20BalanceResults, .success, "")
    }
    self._erc20TokenAllowance = { _, _, _, _, completion in
      completion("0", .success, "")
    }
    self._solanaBalance = { accountAddress, chainId, completion in
      completion(0, .success, "")
    }
    self._splTokenAccountBalance = { _, tokenMintAddress, _, completion in
      completion("0", 0, "", .success, "")
    }
    self._splTokenBalances = { _, _, completion in
      completion([], .success, "")
    }
    self._ethTokenInfo = { _, _, completion in
      completion(nil, .resourceNotFound, "Token not found.")
    }
    self._nftMetadatas = { _, _, completion in
      completion([], "Error Message")
    }
    self._nftBalances = { _, _, _, completion in
      completion([0], "")
    }
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
    supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:))
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
    supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:))
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
    supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:))
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
    supportedKeyrings: [BraveWallet.KeyringId.solana.rawValue].map(NSNumber.init(value:))
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
    supportedKeyrings: [BraveWallet.KeyringId.solana.rawValue].map(NSNumber.init(value:))
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
    supportedKeyrings: [BraveWallet.KeyringId.filecoin.rawValue].map(NSNumber.init(value:))
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
    supportedKeyrings: [BraveWallet.KeyringId.filecoinTestnet.rawValue].map(NSNumber.init(value:))
  )
  static let mockBitcoinMainnet: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.BitcoinMainnet,
    chainName: "Bitcoin Mainnet",
    blockExplorerUrls: ["https://bitcoin.explorer"],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://bitcoin.rpc")!],
    symbol: "BTC",
    symbolName: "Bitcoin",
    decimals: 8,
    coin: .btc,
    supportedKeyrings: [BraveWallet.KeyringId.bitcoin84.rawValue].map(NSNumber.init(value:))
  )
  static let mockBitcoinTestnet: BraveWallet.NetworkInfo = .init(
    chainId: BraveWallet.BitcoinTestnet,
    chainName: "Bitcoin Testnet",
    blockExplorerUrls: ["https://bitcoin.explorer"],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [URL(string: "https://bitcoin.rpc/test")!],
    symbol: "BTC",
    symbolName: "Bitcoin",
    decimals: 8,
    coin: .btc,
    supportedKeyrings: [BraveWallet.KeyringId.bitcoin84Testnet.rawValue].map(NSNumber.init(value:))
  )
}

#endif
