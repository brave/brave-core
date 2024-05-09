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
  /// Return all visible or all invisible user assets in form of `NetworkAssets`
  func getAllUserAssetsInNetworkAssetsByVisibility(
    networks: [BraveWallet.NetworkInfo],
    visible: Bool
  ) -> [NetworkAssets]
  /// Return all user assets in form of `NetworkAssets`
  func getAllUserAssetsInNetworkAssets(
    networks: [BraveWallet.NetworkInfo],
    includingUserDeleted: Bool
  ) -> [NetworkAssets]
  /// Return all spam or non-spam user assets in form of `NetworkAssets`
  func getAllUserNFTs(networks: [BraveWallet.NetworkInfo], isSpam: Bool) -> [NetworkAssets]
  /// Return all user marked deleted user assets
  func getAllUserDeletedNFTs() -> [WalletUserAsset]
  /// Return a `WalletUserAsset` with a given `BraveWallet.BlockchainToken`
  func getUserAsset(_ asset: BraveWallet.BlockchainToken) -> WalletUserAsset?
  /// Add a `WalletUserAsset` representation of the given
  /// `BraveWallet.BlockchainToken` to CoreData
  func addUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?)
  /// Remove a `WalletUserAsset` representation of the given
  /// `BraveWallet.BlockchainToken` from CoreData
  func removeUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?)
  /// Remove an entire `WalletUserAssetGroup` with a given `groupId`
  func removeGroup(for groupId: String, completion: (() -> Void)?)
  /// Update a `WalletUserAsset`'s `visible`, `isSpam`, and `isDeletedByUser` status
  func updateUserAsset(
    for asset: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool,
    completion: (() -> Void)?
  )

  /// Balance
  /// Return balance in String of the given asset. Return nil if there no balance stored
  func getBalances(
    for asset: BraveWallet.BlockchainToken?,
    account: String?
  ) -> [WalletUserAssetBalance]?
  /// Store asset balance if there is none exists. Update asset balance if asset exists in database
  func updateBalance(
    for asset: BraveWallet.BlockchainToken,
    account: String,
    balance: String,
    completion: (() -> Void)?
  )
  /// Remove a `WalletUserAssetBalance` representation of the given
  /// `BraveWallet.BlockchainToken` from CoreData
  func removeBalances(for asset: BraveWallet.BlockchainToken, completion: (() -> Void)?)
  /// Remove a `WalletUserAssetBalance` representation of the given
  /// `BraveWallet.NetworkInfo` from CoreData
  func removeBalances(for network: BraveWallet.NetworkInfo, completion: (() -> Void)?)
  /// Add a user asset data observer
  func addUserAssetDataObserver(_ observer: WalletUserAssetDataObserver)
  /// Remove user assets and their cached balance that belongs to given network
  func removeUserAssetsAndBalance(for network: BraveWallet.NetworkInfo?, completion: (() -> Void)?)
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
  public func getAllUserAssetsInNetworkAssets(
    networks: [BraveWallet.NetworkInfo],
    includingUserDeleted: Bool
  ) -> [NetworkAssets] {
    var allUserAssets: [NetworkAssets] = []
    for (index, network) in networks.enumerated() {
      let groupId = network.walletUserAssetGroupId
      if let walletUserAssets = WalletUserAssetGroup.getGroup(groupId: groupId)?.walletUserAssets?
        .filter({
          if !includingUserDeleted {
            return $0.isDeletedByUser == false
          }
          return true
        })
      {
        let networkAsset = NetworkAssets(
          network: network,
          tokens: walletUserAssets.map(\.blockchainToken),
          sortOrder: index
        )
        allUserAssets.append(networkAsset)
      }
    }
    return allUserAssets.sorted(by: { $0.sortOrder < $1.sortOrder })
  }

  /// Return either all visible or invisible user's assets (no spam) stored in CoreData based on
  /// the given `visible` value
  public func getAllUserAssetsInNetworkAssetsByVisibility(
    networks: [BraveWallet.NetworkInfo],
    visible: Bool
  ) -> [NetworkAssets] {
    var allVisibleUserAssets: [NetworkAssets] = []
    for (index, network) in networks.enumerated() {
      let groupId = network.walletUserAssetGroupId
      if let walletUserAssets = WalletUserAssetGroup.getGroup(groupId: groupId)?.walletUserAssets?
        .filter({ $0.visible == visible && $0.isSpam == false && $0.isDeletedByUser == false })
      {
        let networkAsset = NetworkAssets(
          network: network,
          tokens: walletUserAssets.map(\.blockchainToken),
          sortOrder: index
        )
        allVisibleUserAssets.append(networkAsset)
      }
    }
    return allVisibleUserAssets.sorted(by: { $0.sortOrder < $1.sortOrder })
  }

  /// Return all user NFTs stored in CoreData with the given `isSpam` status
  public func getAllUserNFTs(networks: [BraveWallet.NetworkInfo], isSpam: Bool) -> [NetworkAssets] {
    var allUserSpamAssets: [NetworkAssets] = []
    for (index, network) in networks.enumerated() {
      let groupId = network.walletUserAssetGroupId
      if let walletUserAssets = WalletUserAssetGroup.getGroup(groupId: groupId)?.walletUserAssets?
        .filter({ $0.isSpam == isSpam && ($0.isERC721 || $0.isNFT) && $0.isDeletedByUser == false })
      {
        // Even though users can only spam/unspam NFTs, but we put the NFT filter here to make sure only NFTs are returned
        let networkAsset = NetworkAssets(
          network: network,
          tokens: walletUserAssets.map(\.blockchainToken),
          sortOrder: index
        )
        allUserSpamAssets.append(networkAsset)
      }
    }
    return allUserSpamAssets.sorted(by: { $0.sortOrder < $1.sortOrder })
  }

  public func getAllUserDeletedNFTs() -> [WalletUserAsset] {
    WalletUserAsset.getAllUserDeletedUserAssets() ?? []
  }

  public func getUserAsset(_ asset: BraveWallet.BlockchainToken) -> WalletUserAsset? {
    WalletUserAsset.getUserAsset(asset: asset)
  }

  public func addUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
    if let existedAsset = WalletUserAsset.getUserAsset(asset: asset) {
      if existedAsset.isDeletedByUser {
        // this asset was added before but user marked as deleted after
        WalletUserAsset.updateUserAsset(
          for: asset,
          visible: true,
          isSpam: false,
          isDeletedByUser: false,
          completion: { [weak self] in
            self?.refreshBalance(for: asset)
            self?.retrieveAllDataObserver().forEach { $0.userAssetUpdated() }
            completion?()
          }
        )
      } else {  // this asset already exists
        completion?()
        return
      }
    } else {  // asset does not exist in database
      WalletUserAsset.addUserAsset(
        asset: asset,
        completion: { [weak self] in
          self?.refreshBalance(for: asset)
          self?.retrieveAllDataObserver().forEach { $0.userAssetUpdated() }
          completion?()
        }
      )
    }
  }

  public func removeUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
    WalletUserAsset.removeUserAsset(
      asset: asset,
      completion: { [weak self] in
        self?.removeBalances(for: asset, completion: nil)
        self?.retrieveAllDataObserver().forEach { $0.userAssetUpdated() }
        completion?()
      }
    )
  }

  public func updateUserAsset(
    for asset: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool,
    completion: (() -> Void)?
  ) {
    WalletUserAsset.updateUserAsset(
      for: asset,
      visible: visible,
      isSpam: isSpam,
      isDeletedByUser: isDeletedByUser,
      completion: { [weak self] in
        if visible {
          self?.refreshBalance(for: asset)
        }
        self?.retrieveAllDataObserver().forEach { observer in
          observer.userAssetUpdated()
        }
        completion?()
      }
    )
  }

  public func removeGroup(for groupId: String, completion: (() -> Void)?) {
    WalletUserAssetGroup.removeGroup(
      groupId,
      completion: { [weak self] in
        self?.retrieveAllDataObserver().forEach { $0.userAssetUpdated() }
        completion?()
      }
    )
  }

  public func migrateUserAssets(completion: (() -> Void)? = nil) {
    Task { @MainActor in
      if !Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.value {
        migrateUserAssets(
          for: Array(WalletConstants.supportedCoinTypes()),
          completion: { [weak self] in
            // new wallet created or new wallet restored. finished user asset migration
            // so we want to fetch user assets balances and cache them
            self?.refreshBalances()
            completion?()
          }
        )
      } else {
        let allNetworks = await rpcService.allNetworksForSupportedCoins(
          respectHiddenNetworksPreference: false
        )
        DataController.performOnMainContext { context in
          let newCoins = self.allNewCoinsIntroduced(networks: allNetworks, context: context)
          if !newCoins.isEmpty {
            // new coin type introduced, so we want to fetch user assets balances and cache them after
            // new coin type assets have been migrated to CD
            self.migrateUserAssets(
              for: newCoins,
              completion: { [weak self] in
                self?.refreshBalances()
                completion?()
              }
            )
          } else {
            // no migration happens. refreshing user assets balance will happen after unlock
            completion?()
          }
        }
      }
    }
  }

  public func getBalances(
    for asset: BraveWallet.BlockchainToken?,
    account: String?
  ) -> [WalletUserAssetBalance]? {
    WalletUserAssetBalance.getBalances(for: asset, account: account)
  }

  public func updateBalance(
    for asset: BraveWallet.BlockchainToken,
    account: String,
    balance: String,
    completion: (() -> Void)?
  ) {
    WalletUserAssetBalance.updateBalance(
      for: asset,
      balance: balance,
      account: account,
      completion: completion
    )
  }

  public func removeBalances(for asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
    WalletUserAssetBalance.removeBalances(
      for: asset,
      completion: { [weak self] in
        self?.retrieveAllDataObserver().forEach { $0.cachedBalanceRefreshed() }
        completion?()
      }
    )
  }

  public func removeBalances(for network: BraveWallet.NetworkInfo, completion: (() -> Void)?) {
    WalletUserAssetBalance.removeBalances(
      for: network,
      completion: { [weak self] in
        self?.retrieveAllDataObserver().forEach { $0.cachedBalanceRefreshed() }
        completion?()
      }
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
      let allUserAssets: [NetworkAssets] = self.getAllUserAssetsInNetworkAssets(
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
                  WalletUserAssetBalance.updateBalance(
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
                    WalletUserAssetBalance.updateBalance(
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

  public func removeUserAssetsAndBalance(
    for network: BraveWallet.NetworkInfo?,
    completion: (() -> Void)?
  ) {
    let group = DispatchGroup()
    if let network {
      group.enter()
      WalletUserAssetGroup.removeGroup(network.walletUserAssetGroupId) {
        group.leave()
      }
      group.enter()
      WalletUserAssetBalance.removeBalances(for: network) {
        group.leave()
      }
    } else {
      group.enter()
      WalletUserAssetGroup.removeAllGroup {
        group.leave()
      }
      group.enter()
      WalletUserAssetBalance.removeBalances {
        group.leave()
      }
    }
    group.notify(queue: .main) {
      completion?()
    }
  }

  private func refreshBalance(
    for asset: BraveWallet.BlockchainToken,
    completion: (() -> Void)? = nil
  ) {
    Task { @MainActor in
      let network = await rpcService.allNetworksForSupportedCoins().first {
        $0.chainId.lowercased() == asset.chainId.lowercased()
      }
      guard let network = network else { return }
      let accounts = await keyringService.allAccounts().accounts.filter { $0.coin == asset.coin }

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
                  for: asset,
                  in: account,
                  network: network
                )
              }
              WalletUserAssetBalance.updateBalance(
                for: asset,
                balance: String(tokenBalance ?? 0),
                account: account.id
              )
            }
          }
        }
      )
      retrieveAllDataObserver().forEach { $0.cachedBalanceRefreshed() }
      completion?()
    }
  }

  private func allNewCoinsIntroduced(
    networks: [BraveWallet.NetworkInfo],
    context: NSManagedObjectContext
  ) -> [BraveWallet.CoinType] {
    guard
      let assetGroupIds = WalletUserAssetGroup.getAllGroups(context: context)?.map({ group in
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

  private func migrateUserAssets(for coins: [BraveWallet.CoinType], completion: (() -> Void)?) {
    Task { @MainActor in
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
      WalletUserAsset.migrateVisibleAssets(fetchedUserAssets) {
        Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.value = true
        completion?()
      }
    }
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
}

#if DEBUG
public class TestableWalletUserAssetManager: WalletUserAssetManagerType {
  public var _getAllUserAssetsInNetworkAssetsByVisibility:
    ((_ networks: [BraveWallet.NetworkInfo], _ visible: Bool) -> [NetworkAssets])?
  public var _getAllUserAssetsInNetworkAssets:
    ((_ networks: [BraveWallet.NetworkInfo], _ includingUserDeleted: Bool) -> [NetworkAssets])?
  public var _getAllUserNFTs:
    ((_ networks: [BraveWallet.NetworkInfo], _ spamStatus: Bool) -> [NetworkAssets])?
  public var _getAllUserDeletedNFTs: (() -> [WalletUserAsset])?
  public var _getBalance:
    ((_ asset: BraveWallet.BlockchainToken?, _ account: String?) -> [WalletUserAssetBalance]?)?

  public init() {}

  public func getAllUserAssetsInNetworkAssets(
    networks: [BraveWallet.NetworkInfo],
    includingUserDeleted: Bool
  ) -> [NetworkAssets] {
    let defaultAssets: [NetworkAssets] = [
      NetworkAssets(network: .mockMainnet, tokens: [.previewToken], sortOrder: 0),
      NetworkAssets(network: .mockGoerli, tokens: [.previewToken], sortOrder: 1),
    ]
    let chainIds = networks.map { $0.chainId }
    return _getAllUserAssetsInNetworkAssets?(networks, includingUserDeleted)
      ?? defaultAssets.filter({
        chainIds.contains($0.network.chainId)
      })
  }

  public func getAllUserAssetsInNetworkAssetsByVisibility(
    networks: [BraveWallet.NetworkInfo],
    visible: Bool
  ) -> [NetworkAssets] {
    _getAllUserAssetsInNetworkAssetsByVisibility?(networks, visible) ?? []
  }

  public func getAllUserNFTs(networks: [BraveWallet.NetworkInfo], isSpam: Bool) -> [NetworkAssets] {
    _getAllUserNFTs?(networks, isSpam) ?? []
  }

  public func getAllUserDeletedNFTs() -> [WalletUserAsset] {
    _getAllUserDeletedNFTs?() ?? []
  }

  public func getUserAsset(_ asset: BraveWallet.BlockchainToken) -> WalletUserAsset? {
    return nil
  }

  public func addUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
  }

  public func removeUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
  }

  public func removeGroup(for groupId: String, completion: (() -> Void)?) {
  }

  public func updateUserAsset(
    for asset: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool,
    completion: (() -> Void)?
  ) {
  }

  public func getBalances(
    for asset: BraveWallet.BlockchainToken?,
    account: String?
  ) -> [WalletUserAssetBalance]? {
    _getBalance?(asset, account)
  }

  public func updateBalance(
    for asset: BraveWallet.BlockchainToken,
    account: String,
    balance: String,
    completion: (() -> Void)?
  ) {
    completion?()
  }

  public func removeBalances(for asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
    completion?()
  }

  public func removeBalances(for network: BraveWallet.NetworkInfo, completion: (() -> Void)?) {
    completion?()
  }

  public func addUserAssetDataObserver(_ observer: WalletUserAssetDataObserver) {
  }

  public func removeUserAssetsAndBalance(
    for network: BraveWallet.NetworkInfo?,
    completion: (() -> Void)?
  ) {
    completion?()
  }
}
#endif
