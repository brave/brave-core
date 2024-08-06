// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Data
import Foundation
import Preferences

public protocol WalletUserAssetDataObserver: AnyObject {
  /// This will be trigger when a balance refresh has been finished
  func cachedBalanceRefreshed()
  /// This will be trigger when user asset is updated
  func userAssetUpdated()
}

public protocol WalletUserAssetManagerType: AnyObject {
  /// Return all visible or invisible user assets from `WalletService` that are on given a list of networks in form of `NetworkAssets`
  /// The result will exclude any user assets that are previously marked as deleted by user
  func getUserAssets(
    networks: [BraveWallet.NetworkInfo],
    visible: Bool
  ) async -> [NetworkAssets]
  /// Return all user assets from `WalletService` that are on given a list of networks in form of `NetworkAssets`,
  /// given the option to include or exclude any user assets that are marked as deleted by user
  func getAllUserAssetsInNetworkAssets(
    networks: [BraveWallet.NetworkInfo],
    includingUserDeleted: Bool
  ) async -> [NetworkAssets]
  /// Return all spam or non-spam user assets (NFTs) from `WalletService` that are on given a list of networks in form of `NetworkAssets`
  /// The result will exclude any user assets that are previously marked as deleted by user
  func getAllUserNFTs(
    networks: [BraveWallet.NetworkInfo],
    isSpam: Bool
  ) async -> [NetworkAssets]
  /// Return all user marked deleted user assets
  func getAllUserDeletedUserAssets() -> [WalletUserAsset]
  /// Return a `WalletUserAsset` with a given `BraveWallet.BlockchainToken`
  func getUserAsset(_ asset: BraveWallet.BlockchainToken) -> WalletUserAsset?
  /// Add asset to `WalletService` as well as adding a
  /// `WalletUserAsset` representation in CoreData
  /// This should be only used when user manually adding a custom asset.
  @discardableResult func addUserAsset(
    _ token: BraveWallet.BlockchainToken,
    isAutoDiscovery: Bool
  ) async -> Bool
  /// Remove a `WalletUserAsset` representation of the given
  /// `BraveWallet.BlockchainToken` from CoreData.
  /// As well as update in `WalletService`.
  @discardableResult func removeUserAsset(_ token: BraveWallet.BlockchainToken) async -> Bool
  /// Remove an entire `WalletUserAssetGroup` with a given `groupId`
  func removeGroup(for groupId: String) async
  /// Update a `WalletUserAsset`'s `visible`, `isSpam`, and `isDeletedByUser` status
  /// As well as update in `WalletService`.
  func updateUserAsset(
    for token: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool
  ) async

  /// Balance
  /// Return balance in String of the given asset. Return nil if there no balance stored
  func getAssetBalances(
    for token: BraveWallet.BlockchainToken?,
    account: String?
  ) -> [WalletUserAssetBalance]?
  /// Store asset balance if there is none exists. Update asset balance if asset exists in database
  func updateAssetBalance(
    for token: BraveWallet.BlockchainToken,
    account: String,
    balance: String
  ) async
  /// Remove a `WalletUserAssetBalance` representation of the given
  /// `BraveWallet.BlockchainToken` from CoreData
  func removeAssetBalances(for asset: BraveWallet.BlockchainToken) async
  /// Remove a `WalletUserAssetBalance` representation of the given
  /// `BraveWallet.NetworkInfo` from CoreData
  func removeAssetBalances(for network: BraveWallet.NetworkInfo) async
  /// Add a user asset data observer
  func addUserAssetDataObserver(_ observer: WalletUserAssetDataObserver)
  /// Remove user assets and their cached balance that belongs to given network
  func removeUserAssetsAndBalances(for network: BraveWallet.NetworkInfo?) async
}

public class WalletUserAssetManager: WalletUserAssetManagerType, WalletObserverStore {

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let txService: BraveWalletTxService
  private let bitcoinWalletService: BraveWalletBitcoinWalletService

  private var keyringServiceObserver: KeyringServiceObserver?
  private var txServiceObserver: TxServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?

  let dataObservers = NSHashTable<AnyObject>.weakObjects()

  var isObserving: Bool {
    keyringServiceObserver != nil
      && txServiceObserver != nil
      && walletServiceObserver != nil
  }

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    txService: BraveWalletTxService,
    bitcoinWalletService: BraveWalletBitcoinWalletService
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.txService = txService
    self.bitcoinWalletService = bitcoinWalletService

    setupObservers()
  }

  public func tearDown() {
    keyringServiceObserver = nil
    txServiceObserver = nil
    walletServiceObserver = nil
    dataObservers.removeAllObjects()
  }

  public func setupObservers() {
    guard !isObserving else { return }

    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _unlocked: { [weak self] in
        Task { @MainActor [self] in
          // migrate `account.address` with `account.accountId.uniqueKey` (`account.id`)
          if !Preferences.Wallet.migrateCacheKeyCompleted.value {
            await self?.migrateCacheKey()
            self?.refreshBalances()
          } else {
            self?.refreshBalances()
          }
        }
      },
      _accountsChanged: { [weak self] in
        self?.refreshBalances()
      },
      _accountsAdded: { [weak self] newAccounts in
        self?.refreshBalances()
      }
    )
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onTransactionStatusChanged: { [weak self] txInfo in
        if txInfo.txStatus == .confirmed || txInfo.txStatus == .error || txInfo.txStatus == .dropped
        {
          self?.refreshBalances()
        }
      }
    )
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onNetworkListChanged: { [weak self] in
        self?.refreshBalances {
          self?.retrieveAllDataObserver().forEach {
            $0.cachedBalanceRefreshed()
          }
        }
      }
    )
  }

  /// Return all user's assets stored in CoreData
  @MainActor public func getAllUserAssetsInNetworkAssets(
    networks: [BraveWallet.NetworkInfo],
    includingUserDeleted: Bool
  ) async -> [NetworkAssets] {
    let locallyDeletedTokens: [BraveWallet.BlockchainToken] =
      WalletUserAsset.getAllUserDeletedUserAssets()?
      .map { $0.blockchainToken } ?? []
    let locallyDeletedTokenIds: Set<String> = Set(locallyDeletedTokens.map { $0.id })

    let userAssetsFromService = await walletService.allUserAssets(in: networks)

    var result: [NetworkAssets] = []
    for (index, networkAssetsFromService) in userAssetsFromService.enumerated() {
      let userAssets = networkAssetsFromService.tokens.filter { token in
        if !includingUserDeleted {
          return !locallyDeletedTokenIds.contains { deletedTokenId in
            deletedTokenId.caseInsensitiveCompare(token.id) == .orderedSame
          }
        }
        return true
      }
      let networkAsset = NetworkAssets(
        network: networkAssetsFromService.network,
        tokens: userAssets,
        sortOrder: index
      )
      result.append(networkAsset)
    }
    return result.sorted(by: { $0.sortOrder < $1.sortOrder })
  }

  @MainActor public func getUserAssets(
    networks: [BraveWallet.NetworkInfo],
    visible: Bool
  ) async -> [NetworkAssets] {
    let networkIds = networks.map { $0.chainId }

    let locallyDeletedTokens: [BraveWallet.BlockchainToken] =
      WalletUserAsset.getAllUserDeletedUserAssets()?
      .map { $0.blockchainToken } ?? []
    let locallyDeletedTokenIds: Set<String> = Set(locallyDeletedTokens.map { $0.id.lowercased() })

    let locallyHiddenTokens: [BraveWallet.BlockchainToken] =
      WalletUserAsset.getAllUserAssets(visible: false)?
      .map { $0.blockchainToken } ?? []
    let locallyHiddenTokenIds: Set<String> = Set(locallyHiddenTokens.map { $0.id.lowercased() })

    let userAssetsFromService = await walletService.allUserAssets()
      .filter { networkIds.contains($0.chainId) }

    var result: [NetworkAssets] = []
    for (index, network) in networks.enumerated() {
      let userAssets = userAssetsFromService.filter { token in
        if visible {
          return token.visible
            && !locallyDeletedTokenIds.contains(token.id.lowercased())
            && !locallyHiddenTokenIds.contains(token.id.lowercased())
            && token.chainId == network.chainId
            && token.isSpam == false
        } else {
          return
            (!token.visible
            || locallyHiddenTokenIds.contains(token.id.lowercased()))
            && token.chainId == network.chainId
            && token.isSpam == false
        }
      }
      // It is possible that `visible` flag from `WalletService`
      // and database is out of sync. But we should respect the
      // value from database.
      let updatedUserAssets = userAssets.map {
        let copy = $0
        copy.visible = visible
        return copy
      }
      let networkAsset = NetworkAssets(
        network: network,
        tokens: updatedUserAssets,
        sortOrder: index
      )
      result.append(networkAsset)
    }
    return result.sorted(by: { $0.sortOrder < $1.sortOrder })
  }

  /// Return all user NFTs stored in CoreData with the given `isSpam` status
  @MainActor public func getAllUserNFTs(
    networks: [BraveWallet.NetworkInfo],
    isSpam: Bool
  ) async -> [NetworkAssets] {
    let networkIds = networks.map { $0.chainId }
    let locallyDeletedTokens: [BraveWallet.BlockchainToken] =
      WalletUserAsset.getAllUserDeletedUserAssets()?
      .map { $0.blockchainToken } ?? []

    let userAssetsFromService = await walletService.allUserAssets()
      .filter { networkIds.contains($0.chainId) }

    var result: [NetworkAssets] = []
    for (index, network) in networks.enumerated() {
      let userAssets = userAssetsFromService.filter { token in
        return (token.isNft || token.isErc721)
          && !locallyDeletedTokens.contains(where: {
            $0.id.caseInsensitiveCompare(token.id) == .orderedSame
          })
          && token.chainId == network.chainId
          && token.isSpam == isSpam
      }
      let networkAsset = NetworkAssets(
        network: network,
        tokens: userAssets,
        sortOrder: index
      )
      result.append(networkAsset)
    }
    return result.sorted(by: { $0.sortOrder < $1.sortOrder })
  }

  public func getAllUserDeletedUserAssets() -> [WalletUserAsset] {
    WalletUserAsset.getAllUserDeletedUserAssets() ?? []
  }

  public func getUserAsset(_ asset: BraveWallet.BlockchainToken) -> WalletUserAsset? {
    WalletUserAsset.getUserAsset(token: asset)
  }

  @discardableResult @MainActor public func addUserAsset(
    _ token: BraveWallet.BlockchainToken,
    isAutoDiscovery: Bool
  ) async -> Bool {
    // update database
    if let existedAsset = WalletUserAsset.getUserAsset(token: token) {
      if isAutoDiscovery, existedAsset.isDeletedByUser {
        // Auto-discovered asset but deleted by user
        // do not update database or re-add it
        // to WalletService. And update the visibiliy to
        // `false` in WalletService
        await walletService.setUserAssetVisible(
          token: token,
          visible: false
        )
        return false
      } else if !isAutoDiscovery
        && existedAsset.isDeletedByUser
      {
        // A custom asset or an auto-discovered asset
        // that is deleted by user
        // user adding manually
        await WalletUserAsset.updateUserAsset(
          for: token,
          visible: true,
          isSpam: false,
          isDeletedByUser: false
        )
        await refreshBalance(for: token)

        // update WalletService
        await walletService.removeUserAsset(token: token)
        let success = await walletService.addUserAsset(token: token)

        retrieveAllDataObserver().forEach {
          $0.userAssetUpdated()
        }

        return success
      } else if isAutoDiscovery, !existedAsset.visible {
        // an auto-discovered asset but was previously hidden by user
        // we will need to update WalletService, since WebUI will only read from WalletService,
        // it has no access to database
        await walletService.setUserAssetVisible(
          token: token,
          visible: false
        )
        return false
      } else {
        // 1. auto-discovered is visible and is NOT marked as deleted by user
        // 2. custom asset is either visible or hidden and NOT marked as deleted by user
        return false
      }
    } else {
      // does not exist in database
      // add it to database
      await WalletUserAsset.addUserAsset(token: token)
      await refreshBalance(for: token)

      // update WalletService
      let success: Bool
      if !isAutoDiscovery {
        success = await walletService.addUserAsset(token: token)
      } else {
        // auto-discovery assets already added to WalletService by core
        success = true
      }

      retrieveAllDataObserver().forEach {
        $0.userAssetUpdated()
      }

      return success
    }
  }

  @discardableResult @MainActor public func removeUserAsset(
    _ token: BraveWallet.BlockchainToken
  ) async -> Bool {
    // update database
    await updateUserAsset(
      for: token,
      visible: false,
      isSpam: token.isSpam,
      isDeletedByUser: true
    )
    await removeAssetBalances(for: token)

    // remove user asset from WalletService
    let success = await walletService.removeUserAsset(token: token)

    retrieveAllDataObserver().forEach {
      $0.userAssetUpdated()
    }

    return success
  }

  @MainActor public func updateUserAsset(
    for token: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool
  ) async {
    await WalletUserAsset.updateUserAsset(
      for: token,
      visible: visible,
      isSpam: isSpam,
      isDeletedByUser: isDeletedByUser
    )
    if token.visible != visible {
      // update visibility in WalletService
      let success = await walletService.setUserAssetVisible(
        token: token,
        visible: visible
      )
      if !success, !token.contractAddress.isEmpty {
        // update the custom asset visibilty failed
        // try to add this custom asset
        await walletService.addUserAsset(token: token)
      }
    }
    if visible {
      await refreshBalance(for: token)
    }
    if token.isSpam != isSpam {
      await walletService.setAssetSpamStatus(token: token, status: isSpam)
      if !isSpam {
        await walletService.addUserAsset(token: token)
      }
    }
    retrieveAllDataObserver().forEach { observer in
      observer.userAssetUpdated()
    }
  }

  @MainActor public func removeGroup(for groupId: String) async {
    await WalletUserAssetGroup.removeGroup(
      groupId
    )
    retrieveAllDataObserver().forEach {
      $0.userAssetUpdated()
    }
  }

  @MainActor public func migrateUserAssets() async {
    if !Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.value {
      await migrateUserAssets(
        for: Array(WalletConstants.supportedCoinTypes())
      )
      // new wallet created or new wallet restored. finished user asset migration
      // so we want to fetch user assets balances and cache them
      refreshBalances()
    } else {
      // This wallet has migrated user assets from WalletService
      // to local database

      // Checking if Core has introduced new coin type
      // so that we will need to migrate default tokens from
      // all networks that belong to that new coin type
      let allNetworks = await rpcService.allNetworksForSupportedCoins(
        respectHiddenNetworksPreference: false
      )
      let newCoins = allNewCoinsIntroduced(networks: allNetworks)
      if !newCoins.isEmpty {
        // new coin type introduced, so we want to fetch user assets balances and cache them after
        // new coin type assets have been migrated to CD
        await migrateUserAssets(for: newCoins)
        refreshBalances()
      }

      // For WebUI, we need to migrate any assets that were added by user post
      // `Preferences.Wallet.migrateCoreToWalletUserAssetCompleted`
      // back to WalletService
      if !Preferences.Wallet.migrateWalletUserAssetToCoreCompleted.value {
        let userAssetsFromService = await walletService.allUserAssets()
        let userAssetsFromData =
          WalletUserAsset.getAllUserAssets().map { $0.blockchainToken }
        let newAssets = userAssetsFromData.filter { tokenFromData in
          !userAssetsFromService.contains { tokenFromService in
            tokenFromService.id.caseInsensitiveCompare(tokenFromData.id) == .orderedSame
          }
        }
        await walletService.addUserAssets(newAssets)
        Preferences.Wallet.migrateWalletUserAssetToCoreCompleted.value = true
      }
    }
  }

  public func getAssetBalances(
    for token: BraveWallet.BlockchainToken?,
    account: String?
  ) -> [WalletUserAssetBalance]? {
    WalletUserAssetBalance.getBalances(for: token, account: account)
  }

  @MainActor public func updateAssetBalance(
    for token: BraveWallet.BlockchainToken,
    account: String,
    balance: String
  ) async {
    await WalletUserAssetBalance.updateBalance(
      for: token,
      balance: balance,
      account: account
    )
  }

  @MainActor public func removeAssetBalances(
    for token: BraveWallet.BlockchainToken
  ) async {
    await WalletUserAssetBalance.removeBalances(
      for: token
    )
    retrieveAllDataObserver().forEach {
      $0.cachedBalanceRefreshed()
    }
  }

  @MainActor public func removeAssetBalances(
    for network: BraveWallet.NetworkInfo
  ) async {
    await WalletUserAssetBalance.removeBalances(
      for: network
    )
  }

  public func addUserAssetDataObserver(_ observer: WalletUserAssetDataObserver) {
    dataObservers.add(observer)
  }

  private var refreshBalanceTask: Task<Void, Never>?
  public func refreshBalances(_ completion: (() -> Void)? = nil) {
    refreshBalanceTask?.cancel()
    refreshBalanceTask = Task { @MainActor in
      let accounts = await keyringService.allAccountInfos()
      let allNetworks = await rpcService.allNetworksForSupportedCoins()
      let allUserAssets: [NetworkAssets] = await self.getAllUserAssetsInNetworkAssets(
        networks: allNetworks,
        includingUserDeleted: false
      )
      await withTaskGroup(
        of: Void.self,
        body: { @MainActor [rpcService, bitcoinWalletService] group in
          for account in accounts {
            guard !Task.isCancelled else { return }
            group.addTask { @MainActor in
              if account.coin == .btc {
                let networkAssets = allUserAssets.first {
                  $0.network.supportedKeyrings.contains(account.keyringId.rawValue as NSNumber)
                }
                if let btc = networkAssets?.tokens.first,
                  let btcBalance = await bitcoinWalletService.fetchBTCBalance(
                    accountId: account.accountId,
                    type: .total
                  )
                {
                  await WalletUserAssetBalance.updateBalance(
                    for: btc,
                    balance: String(btcBalance),
                    account: account.id
                  )
                }
              } else {
                let allTokenBalanceForAccount = await rpcService.fetchBalancesForTokens(
                  account: account,
                  networkAssets: allUserAssets
                )
                for tokenBalanceForAccount in allTokenBalanceForAccount {
                  guard !Task.isCancelled else { return }
                  let networkAssets = allUserAssets.first { oneNetworkAssets in
                    oneNetworkAssets.tokens.contains { asset in
                      asset.id == tokenBalanceForAccount.key
                    }
                  }
                  if let token = networkAssets?.tokens.first(where: {
                    $0.id == tokenBalanceForAccount.key
                  }) {
                    await WalletUserAssetBalance.updateBalance(
                      for: token,
                      balance: String(tokenBalanceForAccount.value),
                      account: account.id
                    )
                  }
                }
              }
            }
          }
        }
      )
      retrieveAllDataObserver().forEach { $0.cachedBalanceRefreshed() }
      completion?()
    }
  }

  public func removeUserAssetsAndBalances(
    for network: BraveWallet.NetworkInfo?
  ) async {
    await withTaskGroup(of: Void.self) { group in
      group.addTask {
        if let network {
          await WalletUserAssetGroup.removeGroup(network.walletUserAssetGroupId)
          await WalletUserAssetBalance.removeBalances(for: network)
        } else {
          await WalletUserAssetGroup.removeAllGroup()
          await WalletUserAssetBalance.removeBalances()
        }
      }
    }
  }

  @MainActor private func refreshBalance(
    for token: BraveWallet.BlockchainToken
  ) async {
    let network = await rpcService.allNetworksForSupportedCoins().first {
      $0.chainId.lowercased() == token.chainId.lowercased()
    }
    guard let network = network else { return }
    let accounts = await keyringService.allAccounts().accounts.filter { $0.coin == token.coin }
    await withTaskGroup(
      of: Void.self,
      body: { @MainActor [rpcService, bitcoinWalletService] group in
        for account in accounts {  // for each account
          group.addTask { @MainActor in
            var tokenBalance: Double?
            if account.coin == .btc {
              tokenBalance = await bitcoinWalletService.fetchBTCBalance(
                accountId: account.accountId,
                type: .total
              )
            } else {
              tokenBalance = await rpcService.balance(
                for: token,
                in: account,
                network: network
              )
            }
            await WalletUserAssetBalance.updateBalance(
              for: token,
              balance: String(tokenBalance ?? 0),
              account: account.id
            )
          }
        }
      }
    )
    retrieveAllDataObserver().forEach { $0.cachedBalanceRefreshed() }
  }

  private func allNewCoinsIntroduced(
    networks: [BraveWallet.NetworkInfo]
  ) -> [BraveWallet.CoinType] {
    guard
      let assetGroupIds = WalletUserAssetGroup.getAllGroups()?.map({ group in
        group.groupId
      })
    else { return WalletConstants.supportedCoinTypes().elements }
    var newCoins: Set<BraveWallet.CoinType> = []
    for network in networks
    where !assetGroupIds.contains("\(network.coin.rawValue).\(network.chainId)") {
      newCoins.insert(network.coin)
    }
    return Array(newCoins)
  }

  private func migrateUserAssets(for coins: [BraveWallet.CoinType]) async {
    var fetchedUserAssets: [String: [BraveWallet.BlockchainToken]] = [:]
    let networks: [BraveWallet.NetworkInfo] = await rpcService.allNetworks(
      for: coins,
      respectHiddenNetworksPreference: false
    )
    let networkAssets = await walletService.allUserAssets(in: networks)
    for networkAsset in networkAssets {
      fetchedUserAssets["\(networkAsset.network.coin.rawValue).\(networkAsset.network.chainId)"] =
        networkAsset.tokens
    }
    await WalletUserAsset.migrateVisibleAssets(fetchedUserAssets)
    Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.value = true
  }

  private func retrieveAllDataObserver() -> [WalletUserAssetDataObserver] {
    return dataObservers.allObjects as? [WalletUserAssetDataObserver] ?? []
  }

  @MainActor private func migrateCacheKey() async {
    let allAccounts = await keyringService.allAccountInfos()
    // balance
    if let allBalances = WalletUserAssetBalance.getBalances() {
      await withTaskGroup(
        of: Void.self,
        body: { @MainActor group in
          for balance in allBalances {
            if let account = allAccounts.first(where: { $0.address == balance.accountAddress }) {
              group.addTask { @MainActor in
                await WalletUserAssetBalance.updateAccountAddress(for: account)
              }
            }
          }
        }
      )
    }
    // nonSelectedAccountsFilter
    var newAddresses: [String] = []
    for address in Preferences.Wallet.nonSelectedAccountsFilter.value {
      if let account = allAccounts.first(where: { $0.address == address }) {
        newAddresses.append(account.id)
      }
    }
    Preferences.Wallet.nonSelectedAccountsFilter.value = newAddresses

    Preferences.Wallet.migrateCacheKeyCompleted.value = true
  }

  @MainActor public func alignTokenVisibilityServiceAndCD() async {
    let locallyHiddenTokens: [BraveWallet.BlockchainToken] =
      WalletUserAsset.getAllUserAssets(visible: false)?
      .map { $0.blockchainToken } ?? []
    let visibleTokensFromService =
      await walletService.allUserAssets()
      .filter { $0.visible }
    let tokensFromServiceNeedUpdate =
      visibleTokensFromService
      .filter { token in
        locallyHiddenTokens.contains { localHiddenToken in
          localHiddenToken.id.caseInsensitiveCompare(token.id) == .orderedSame
        }
      }
    await withTaskGroup(of: Void.self) { @MainActor group in
      for token in tokensFromServiceNeedUpdate {
        group.addTask { @MainActor in
          await self.walletService.setUserAssetVisible(
            token: token,
            visible: false
          )
        }
      }
    }
  }
}

#if DEBUG
public class TestableWalletUserAssetManager: WalletUserAssetManagerType {
  public var _getUserAssets:
    ((_ networks: [BraveWallet.NetworkInfo], _ visible: Bool) -> [NetworkAssets])?
  public var _getAllUserAssetsInNetworkAssets:
    ((_ networks: [BraveWallet.NetworkInfo], _ includingUserDeleted: Bool) -> [NetworkAssets])?
  public var _getAllUserNFTs:
    ((_ networks: [BraveWallet.NetworkInfo], _ spamStatus: Bool) -> [NetworkAssets])?
  public var _getAllUserDeletedUserAssets: (() -> [WalletUserAsset])?
  public var _getBalance:
    ((_ asset: BraveWallet.BlockchainToken?, _ account: String?) -> [WalletUserAssetBalance]?)?

  public init() {}

  public func getUserAssets(
    networks: [BraveWallet.NetworkInfo],
    visible: Bool
  ) async -> [NetworkAssets] {
    _getUserAssets?(networks, visible) ?? []
  }

  public func getAllUserAssetsInNetworkAssets(
    networks: [BraveWallet.NetworkInfo],
    includingUserDeleted: Bool
  ) -> [NetworkAssets] {
    let defaultAssets: [NetworkAssets] = [
      NetworkAssets(network: .mockMainnet, tokens: [.previewToken], sortOrder: 0),
      NetworkAssets(
        network: .mockSepolia,
        tokens: [
          (BraveWallet.BlockchainToken.previewToken.copy() as! BraveWallet.BlockchainToken).then {
            $0.chainId = BraveWallet.SepoliaChainId
          }
        ],
        sortOrder: 1
      ),
    ]
    let chainIds = networks.map { $0.chainId }
    return _getAllUserAssetsInNetworkAssets?(networks, includingUserDeleted)
      ?? defaultAssets.filter({
        chainIds.contains($0.network.chainId)
      })
  }

  public func getAllUserNFTs(
    networks: [BraveWallet.NetworkInfo],
    isSpam: Bool
  ) async -> [NetworkAssets] {
    _getAllUserNFTs?(networks, isSpam) ?? []
  }

  public func getAllUserDeletedUserAssets() -> [WalletUserAsset] {
    _getAllUserDeletedUserAssets?() ?? []
  }

  public func getUserAsset(_ token: BraveWallet.BlockchainToken) -> WalletUserAsset? {
    return nil
  }

  public func addUserAsset(
    _ token: BraveWallet.BlockchainToken,
    isAutoDiscovery: Bool
  ) async -> Bool {
    return false
  }

  public func removeUserAsset(_ asset: BraveWallet.BlockchainToken) async -> Bool {
    return false
  }

  public func removeGroup(for groupId: String) async {
  }

  public func updateUserAsset(
    for token: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool
  ) async {
  }

  public func getAssetBalances(
    for token: BraveWallet.BlockchainToken?,
    account: String?
  ) -> [WalletUserAssetBalance]? {
    _getBalance?(token, account)
  }

  public func updateAssetBalance(
    for token: BraveWallet.BlockchainToken,
    account: String,
    balance: String
  ) async {
  }

  public func removeAssetBalances(
    for asset: BraveWallet.BlockchainToken
  ) async {
  }

  public func removeAssetBalances(
    for network: BraveWallet.NetworkInfo
  ) async {
  }

  public func addUserAssetDataObserver(_ observer: WalletUserAssetDataObserver) {
  }

  public func removeUserAssetsAndBalances(
    for network: BraveWallet.NetworkInfo?
  ) async {
  }
}
#endif
