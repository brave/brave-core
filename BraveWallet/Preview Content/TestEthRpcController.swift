/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveCore

extension EthNetworkStore {
  static var previewStore: EthNetworkStore {
    .init(ethJsonRpcController: TestEthJsonRpcController())
  }
}

/// A test eth json controller which can be passed to a ``EthNetworkStore`` that implements some basic
/// functionality for the use of SwiftUI Previews.
///
/// - note: Do not use this directly, use ``EthNetworkStore.previewStore``
private class TestEthJsonRpcController: NSObject, BraveWalletEthJsonRpcController {
  private var chainId: String = BraveWallet.MainnetChainId
  private var networks: [BraveWallet.EthereumChain] = [.mainnet, .rinkeby, .ropsten]
  private var networkURL: URL?
  private var observers: NSHashTable<BraveWalletEthJsonRpcControllerObserver> = .weakObjects()
  
  func chainId(_ completion: @escaping (String) -> Void) {
    completion(chainId)
  }
  
  func blockTrackerUrl(_ completion: @escaping (String) -> Void) {
    completion(networks.first(where: { $0.chainId == self.chainId })?.blockExplorerUrls.first ?? "")
  }
  
  func networkUrl(_ completion: @escaping (String) -> Void) {
    completion(networkURL?.absoluteString ?? "")
  }
  
  func balance(_ address: String, completion: @escaping (Bool, String) -> Void) {
    completion(true, "10")
  }
  
  func erc20TokenBalance(_ contract: String, address: String, completion: @escaping (Bool, String) -> Void) {
    completion(true, "10")
  }
  
  func unstoppableDomainsProxyReaderGetMany(_ contractAddress: String, domain: String, keys: [String], completion: @escaping (Bool, String) -> Void) {
    completion(false, "")
  }
  
  func ensProxyReaderGetResolverAddress(_ contractAddress: String, domain: String, completion: @escaping (Bool, String) -> Void) {
    completion(false, "")
  }
  
  func request(_ jsonPayload: String, autoRetryOnNetworkChange: Bool, completion: @escaping (Int32, String, [String: String]) -> Void) {
    completion(0, "", [:])
  }
  
  func add(_ observer: BraveWalletEthJsonRpcControllerObserver) {
    observers.add(observer)
  }
  
  func add(_ chain: BraveWallet.EthereumChain, origin: URL, completion: @escaping (String, Bool) -> Void) {
    self.networks.append(chain)
  }
  
  func addEthereumChainRequestCompleted(_ chainId: String, approved: Bool) {
    
  }
  
  func pendingChainRequests(_ completion: @escaping ([BraveWallet.EthereumChain]) -> Void) {
    completion([])
  }
  
  func allNetworks(_ completion: @escaping ([BraveWallet.EthereumChain]) -> Void) {
    completion(networks)
  }
  
  func setCustomNetworkForTesting(_ chainId: String, providerUrl: URL) {
    
  }
  
  func setNetwork(_ chainId: String) {
    self.chainId = chainId
  }
}

extension BraveWallet.EthereumChain {
  static let mainnet: BraveWallet.EthereumChain = .init(
    chainId: BraveWallet.MainnetChainId,
    chainName: "Mainnet",
    blockExplorerUrls: ["https://etherscan.io"],
    iconUrls: [],
    rpcUrls: [],
    symbol: "",
    symbolName: "",
    decimals: 0
  )
  static let rinkeby: BraveWallet.EthereumChain = .init(
    chainId: BraveWallet.RinkebyChainId,
    chainName: "Rinkeby",
    blockExplorerUrls: ["https://rinkeby.etherscan.io"],
    iconUrls: [],
    rpcUrls: [],
    symbol: "",
    symbolName: "",
    decimals: 0
  )
  static let ropsten: BraveWallet.EthereumChain = .init(
    chainId: BraveWallet.RopstenChainId,
    chainName: "Ropsten",
    blockExplorerUrls: ["https://ropsten.etherscan.io"],
    iconUrls: [],
    rpcUrls: [],
    symbol: "",
    symbolName: "",
    decimals: 0
  )
}
