// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import SwiftUI

struct AccountDetails: Equatable, Identifiable {
  var id: String { account.id }
  let account: BraveWallet.AccountInfo
  let tokensWithBalance: [BraveWallet.BlockchainToken]
  let totalBalanceFiat: String
}

class AccountsStore: ObservableObject, WalletObserverStore {

  /// Users primary accounts
  @Published var primaryAccounts: [AccountDetails] = []
  /// Users imported accounts
  @Published var importedAccounts: [AccountDetails] = []
  /// If we are loading prices or balances
  @Published var isLoading: Bool = false
  /// Users selected currency code
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard oldValue != currencyCode else { return }
      update()
    }
  }

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter

  private typealias TokenBalanceCache = [String: [String: Double]]
  /// Cache of token balances for each account. `[account.id: [token.id: balance]]`
  private var tokenBalancesCache: TokenBalanceCache = [:]
  /// Cache of prices for each token. The key is the token's `assetRatioId`.
  private var pricesCache: [String: String] = [:]

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
  private let userAssetManager: WalletUserAssetManagerType

  private var keyringServiceObserver: KeyringServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?

  var isObserving: Bool {
    keyringServiceObserver != nil && walletServiceObserver != nil
  }

  init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    bitcoinWalletService: BraveWalletBitcoinWalletService,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.bitcoinWalletService = bitcoinWalletService
    self.userAssetManager = userAssetManager
    self.setupObservers()
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.userAssetManager.addUserAssetDataObserver(self)
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _accountsChanged: { [weak self] in
        self?.update()
      }
    )
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onNetworkListChanged: { [weak self] in
        self?.update()
      }
    )
  }

  func tearDown() {
    keyringServiceObserver = nil
    walletServiceObserver = nil
  }

  private var updateTask: Task<(), Never>?
  func update() {
    updateTask?.cancel()
    updateTask = Task { @MainActor in
      self.isLoading = true
      defer { self.isLoading = false }

      let allNetworks = await rpcService.allNetworksForSupportedCoins()
      let allTokensPerNetwork = await userAssetManager.getAllUserAssetsInNetworkAssets(
        networks: allNetworks,
        includingUserDeleted: false
      ).map { networkAssets in  // filter out NFTs
        NetworkAssets(
          network: networkAssets.network,
          tokens: networkAssets.tokens.filter { !($0.isNft || $0.isErc721) },
          sortOrder: networkAssets.sortOrder
        )
      }
      let tokens = allTokensPerNetwork.flatMap(\.tokens)

      var allAccounts = await keyringService.allAccountInfos()
      var accountDetails = buildAccountDetails(accounts: allAccounts, tokens: tokens)
      self.primaryAccounts =
        accountDetails
        .filter(\.account.isPrimary)
      self.importedAccounts =
        accountDetails
        .filter(\.account.isImported)

      await updateBalancesAndPrices(
        for: allAccounts,
        networkAssets: allTokensPerNetwork
      )

      // if new accounts added while balances were being fetched.
      allAccounts = await keyringService.allAccountInfos()
      accountDetails = buildAccountDetails(accounts: allAccounts, tokens: tokens)
      self.primaryAccounts =
        accountDetails
        .filter(\.account.isPrimary)
      self.importedAccounts =
        accountDetails
        .filter(\.account.isImported)
    }
  }

  @MainActor private func updateBalancesAndPrices(
    for accounts: [BraveWallet.AccountInfo],
    networkAssets allNetworkAssets: [NetworkAssets]
  ) async {
    // Update BTC account balance
    if accounts.contains(where: { $0.coin == .btc }) {
      await withTaskGroup(
        of: [String: [String: Double]].self
      ) { [bitcoinWalletService] group in
        for account in accounts where account.coin == .btc {
          group.addTask {
            let btcBalance =
              await bitcoinWalletService.fetchBTCBalance(
                accountId: account.accountId,
                type: .total
              ) ?? 0
            if let btcToken = allNetworkAssets.first(where: {
              $0.network.supportedKeyrings.contains(
                account.keyringId.rawValue as NSNumber
              )
            })?.tokens.first {
              return [account.id: [btcToken.id: btcBalance]]
            }
            return [:]
          }
        }
        for await accountBTCBalances in group {
          tokenBalancesCache.merge(with: accountBTCBalances)
        }
      }
    }
    // Update non-BTC account balance
    let balancesForAccounts = await withTaskGroup(
      of: TokenBalanceCache.self,
      body: { group in
        for account in accounts where account.coin != .btc {
          if let allTokenBalance = self.userAssetManager.getAssetBalances(
            for: nil,
            account: account.id
          ) {
            var result: [String: Double] = [:]
            for balancePerToken in allTokenBalance {
              let tokenId =
                balancePerToken.contractAddress + balancePerToken.chainId
                + balancePerToken.symbol + balancePerToken.tokenId
              result.merge(with: [
                tokenId: Double(balancePerToken.balance) ?? 0
              ])
            }
            self.tokenBalancesCache.merge(with: [account.id: result])
          } else {
            // 1. We have a user asset from CD but wallet has never
            // fetched it's balance. Should never happen. But we will fetch its
            // balance and cache it in CD.
            // 2. Test Cases will come here, we will fetch balance using
            // a mock `rpcService` and `bitcoinWalletService`
            group.addTask {
              var balancesForTokens: [String: Double] = [:]
              balancesForTokens = await self.rpcService.fetchBalancesForTokens(
                account: account,
                networkAssets: allNetworkAssets
              )
              return [account.id: balancesForTokens]
            }
          }
        }

        return await group.reduce(
          into: TokenBalanceCache(),
          { partialResult, new in
            partialResult.merge(with: new)
          }
        )
      }
    )
    for account in accounts {
      if let updatedBalancesForAccount = balancesForAccounts[account.id] {
        // if balance fetch failed that we already have cached, don't overwrite existing
        if var existing = self.tokenBalancesCache[account.id] {
          existing.merge(with: updatedBalancesForAccount)
          self.tokenBalancesCache[account.id] = existing
        } else {
          self.tokenBalancesCache[account.id] = updatedBalancesForAccount
        }
      }
    }
    // fetch prices for tokens with balance
    var tokensIdsWithBalance: Set<String> = .init()
    for accountBalance in tokenBalancesCache.values {
      let tokenIdsWithAccountBalance = accountBalance.filter { $1 > 0 }.map(\.key)
      tokenIdsWithAccountBalance.forEach { tokensIdsWithBalance.insert($0) }
    }
    let allTokens = allNetworkAssets.flatMap(\.tokens)
    let assetRatioIdsForTokensWithBalance =
      tokensIdsWithBalance
      .compactMap { tokenId in
        allTokens.first(where: { $0.id == tokenId })?.assetRatioId
      }
    let prices: [String: String] = await assetRatioService.fetchPrices(
      for: assetRatioIdsForTokensWithBalance,
      toAssets: [currencyFormatter.currencyCode],
      timeframe: .live
    )
    self.pricesCache.merge(with: prices)
  }

  // MARK: Helpers

  private func buildAccountDetails(
    accounts: [BraveWallet.AccountInfo],
    tokens: [BraveWallet.BlockchainToken]
  ) -> [AccountDetails] {
    accounts
      .map { account in
        let tokensWithBalance = tokensSortedByFiat(
          for: account,
          tokens: tokens
        )
        let totalBalanceFiat =
          currencyFormatter.formatAsFiat(
            totalBalanceFiat(
              for: account,
              tokens: tokens
            )
          ) ?? ""
        return AccountDetails(
          account: account,
          tokensWithBalance: tokensWithBalance,
          totalBalanceFiat: totalBalanceFiat
        )
      }
  }

  /// Returns the tokens with a balance for the given account.
  private func tokensSortedByFiat(
    for account: BraveWallet.AccountInfo,
    tokens: [BraveWallet.BlockchainToken]
  ) -> [BraveWallet.BlockchainToken] {
    guard let tokenBalancesForAccount = tokenBalancesCache[account.id] else {
      return []
    }
    var tokensFiatForAccount: [(token: BraveWallet.BlockchainToken, fiat: Double)] = []
    for (tokenId, balance) in tokenBalancesForAccount where balance > 0 {
      guard let token = tokens.first(where: { $0.id == tokenId }) else { continue }
      if let priceString = pricesCache[token.assetRatioId.lowercased()],
        let price = Double(priceString)
      {
        let fiat = balance * price
        tokensFiatForAccount.append((token, fiat))
      }  // else price unknown, can't determine fiat.
    }
    return
      tokensFiatForAccount
      .sorted(by: { $0.fiat > $1.fiat })
      .map(\.token)
  }

  /// Returns the fiat for tokens with a balance for the given account.
  private func totalBalanceFiat(
    for account: BraveWallet.AccountInfo,
    tokens: [BraveWallet.BlockchainToken]
  ) -> Double {
    guard let accountBalanceCache = tokenBalancesCache[account.id] else { return 0 }
    return accountBalanceCache.keys.reduce(0.0) { partialResult, tokenId in
      guard let tokenBalanceForAccount = tokenBalanceForAccount(tokenId: tokenId, account: account)
      else {
        // no balances cached for this token
        // or no balance cached for this account
        return partialResult
      }
      guard let token = tokens.first(where: { $0.id == tokenId }),
        let priceString = pricesCache[token.assetRatioId.lowercased()],
        let price = Double(priceString)
      else {
        // price for token unavailable
        return partialResult
      }
      return partialResult + (tokenBalanceForAccount * price)
    }
  }

  // Helper to get token balance for a given token id and account from cache.
  private func tokenBalanceForAccount(
    tokenId: String,
    account: BraveWallet.AccountInfo
  ) -> Double? {
    tokenBalancesCache[account.id]?[tokenId]
  }
}

extension AccountsStore: WalletUserAssetDataObserver {
  func cachedBalanceRefreshed() {
    update()
  }

  func userAssetUpdated() {
  }
}
