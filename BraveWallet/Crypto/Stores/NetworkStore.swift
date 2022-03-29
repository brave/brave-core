/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveCore
import SwiftUI
import Shared

/// An interface that helps you interact with a json-rpc service
///
/// This wraps a JsonRpcService that you would obtain through BraveCore and makes it observable
public class NetworkStore: ObservableObject {
  @Published private(set) var ethereumChains: [BraveWallet.NetworkInfo] = []
  @Published private(set) var selectedChainId: String = BraveWallet.MainnetChainId

  var selectedChain: BraveWallet.NetworkInfo {
    ethereumChains.first(where: { $0.chainId == self.selectedChainId }) ?? .init()
  }

  var selectedChainBinding: Binding<BraveWallet.NetworkInfo> {
    .init(
      get: { self.ethereumChains.first(where: { $0.chainId == self.selectedChainId }) ?? .init() },
      set: {
        self.selectedChainId = $0.chainId
        self.rpcService.setNetwork($0.chainId) { _ in }
      }
    )
  }

  private let rpcService: BraveWalletJsonRpcService

  public init(rpcService: BraveWalletJsonRpcService) {
    self.rpcService = rpcService
    self.updateChainList()
    rpcService.chainId { chainId in
      let id = chainId.isEmpty ? BraveWallet.MainnetChainId : chainId
      self.selectedChainId = id
      self.rpcService.setNetwork(id) { _ in }
    }
    rpcService.add(self)
  }

  private func updateChainList() {
    rpcService.allNetworks { [self] chains in
      ethereumChains = chains.filter {
        $0.chainId != BraveWallet.LocalhostChainId
      }
    }
  }

  // MARK: - Custom Networks

  @Published var isAddingNewNetwork: Bool = false

  public func addCustomNetwork(
    _ network: BraveWallet.NetworkInfo,
    completion: @escaping (_ accepted: Bool, _ errMsg: String) -> Void
  ) {
    func add(network: BraveWallet.NetworkInfo, completion: @escaping (_ accepted: Bool, _ errMsg: String) -> Void) {
      rpcService.addEthereumChain(network) { [self] chainId, status, message in
        if status == .success {
          // Update `ethereumChains` by api calling
          updateChainList()
          isAddingNewNetwork = false
          completion(true, "")
        } else {
          // meaning add custom network failed for some reason.
          // Also add the the old network back on rpc service
          if let oldNetwork = ethereumChains.first(where: { $0.id.lowercased() == network.id.lowercased() }) {
            rpcService.addEthereumChain(oldNetwork) { _, _, _ in
              // Update `ethereumChains` by api calling
              self.updateChainList()
              self.isAddingNewNetwork = false
              completion(false, message)
            }
          } else {
            isAddingNewNetwork = false
            completion(false, message)
          }
        }
      }
    }

    isAddingNewNetwork = true
    if ethereumChains.contains(where: { $0.id.lowercased() == network.id.lowercased() }) {
      removeNetworkForNewAddition(network) { [self] success in
        guard success else {
          isAddingNewNetwork = false
          completion(false, Strings.Wallet.failedToRemoveCustomNetworkErrorMessage)
          return
        }
        add(network: network, completion: completion)
      }
    } else {
      add(network: network, completion: completion)
    }
  }

  /// This method will not update `ethereumChains`
  private func removeNetworkForNewAddition(
    _ network: BraveWallet.NetworkInfo,
    completion: @escaping (_ success: Bool) -> Void
  ) {
    rpcService.removeEthereumChain(network.id) { success in
      completion(success)
    }
  }

  public func removeCustomNetwork(
    _ network: BraveWallet.NetworkInfo,
    completion: @escaping (_ success: Bool) -> Void
  ) {
    rpcService.removeEthereumChain(network.id) { [self] success in
      if success {
        // check if its the current network, set mainnet the active net
        if network.id.lowercased() == selectedChainId.lowercased() {
          rpcService.setNetwork(BraveWallet.MainnetChainId, completion: { _ in })
        }
        // Update `ethereumChains` by api calling
        updateChainList()
      }
      completion(success)
    }
  }
}

extension NetworkStore: BraveWalletJsonRpcServiceObserver {
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
    self.selectedChainId = chainId
  }
}
