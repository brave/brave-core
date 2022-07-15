/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveCore
import SwiftUI
import Strings

/// An interface that helps you interact with a json-rpc service
///
/// This wraps a JsonRpcService that you would obtain through BraveCore and makes it observable
public class NetworkStore: ObservableObject {
  
  enum SetSelectedChainError: Error {
    case selectedChainHasNoAccounts
    case chainAlreadySelected
    case unknown
  }
  
  @Published private(set) var allChains: [BraveWallet.NetworkInfo] = []

  @Published private(set) var selectedChainId: String = BraveWallet.MainnetChainId
  var selectedChain: BraveWallet.NetworkInfo {
    allChains.first(where: { $0.chainId == self.selectedChainId }) ?? .init()
  }

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.updateChainList()
    rpcService.add(self)
    keyringService.add(self)
    
    Task { @MainActor in // fetch current selected network
      let selectedCoin = await walletService.selectedCoin()
      let chain = await rpcService.network(selectedCoin)
      await setSelectedChain(chain)
    }
  }

  private func updateChainList() {
    Task { @MainActor in // fetch all networks for all coin types
      self.allChains = await withTaskGroup(of: [BraveWallet.NetworkInfo].self) { [weak rpcService] group -> [BraveWallet.NetworkInfo] in
        guard let rpcService = rpcService else { return [] }
        for coinType in WalletConstants.supportedCoinTypes {
          group.addTask {
            let chains = await rpcService.allNetworks(coinType)
            return chains.filter { // localhost not supported
              $0.chainId != BraveWallet.LocalhostChainId
            }
          }
        }
        let allChains = await group.reduce([BraveWallet.NetworkInfo](), { $0 + $1 })
        return allChains.sorted { lhs, rhs in
          // sort solana chains to the front of the list
          lhs.coin == .sol && rhs.coin != .sol
        }
      }
    }
  }
  
  @MainActor @discardableResult func setSelectedChain(_ network: BraveWallet.NetworkInfo) async -> SetSelectedChainError? {
    let keyringId = network.coin.keyringId
    let keyringInfo = await keyringService.keyringInfo(keyringId)
    if keyringInfo.accountInfos.isEmpty {
      // Need to prompt user to create new account via alert
      return .selectedChainHasNoAccounts
    } else {
      guard self.selectedChainId != network.chainId else { return .chainAlreadySelected }
      self.selectedChainId = network.chainId
      let success = await rpcService.setNetwork(network.chainId, coin: network.coin)
      return success ? nil : .unknown
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
          if let oldNetwork = allChains.filter({ $0.coin == .eth }).first(where: { $0.id.lowercased() == network.id.lowercased() }) {
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
    if allChains.filter({ $0.coin == .eth }).contains(where: { $0.id.lowercased() == network.id.lowercased() }) {
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
    guard network.coin == .eth else {
      completion(false)
      return
    }
    rpcService.removeEthereumChain(network.id) { [self] success in
      if success {
        // check if its the current network, set mainnet the active net
        if network.id.lowercased() == selectedChainId.lowercased() {
          rpcService.setNetwork(BraveWallet.MainnetChainId, coin: .eth, completion: { _ in })
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
    updateChainList()
  }
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
    walletService.setSelectedCoin(coin)
    Task { @MainActor in
      guard let chain = allChains.first(where: { $0.chainId == chainId && $0.coin == coin }) else {  return }
      await setSelectedChain(chain)
    }
  }
}

extension NetworkStore: BraveWalletKeyringServiceObserver {
  public func selectedAccountChanged(_ coin: BraveWallet.CoinType) {
    Task { @MainActor in
      // coin type of selected account might have changed
      let chain = await rpcService.network(coin)
      await setSelectedChain(chain)
    }
  }
  
  public func keyringCreated(_ keyringId: String) {
    Task { @MainActor in
      // select the newly created account
      let keyring = await keyringService.keyringInfo(keyringId)
      guard let newAccount = keyring.accountInfos.first else { return }
      await keyringService.setSelectedAccount(newAccount.address, coin: newAccount.coin)
    }
  }
  
  public func keyringRestored(_ keyringId: String) {
  }
  
  public func keyringReset() {
  }
  
  public func locked() {
  }
  
  public func unlocked() {
  }
  
  public func backedUp() {
  }
  
  public func accountsChanged() {
  }
  
  public func autoLockMinutesChanged() {
  }
}
