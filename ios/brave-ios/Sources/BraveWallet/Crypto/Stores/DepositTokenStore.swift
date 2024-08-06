// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

struct DepositTokenViewModel: Identifiable {
  let token: BraveWallet.BlockchainToken
  let network: BraveWallet.NetworkInfo
  var id: String {
    token.id
  }
}

class DepositTokenStore: ObservableObject, WalletObserverStore {
  @Published var networkFilters: [Selectable<BraveWallet.NetworkInfo>] = [] {
    didSet {
      guard !oldValue.isEmpty else { return }  // initial assignment to `networkFilters`
      update()
    }
  }
  @Published var assetViewModels: [DepositTokenViewModel] = []

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private var allTokens: [BraveWallet.BlockchainToken] = []
  private let assetManager: WalletUserAssetManagerType
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
  private var keyringServiceObserver: KeyringServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?

  @Published var prefilledToken: BraveWallet.BlockchainToken?
  var prefilledAccount: BraveWallet.AccountInfo?
  @Published var allAccounts: [BraveWallet.AccountInfo] = []
  @Published var allNetworks: [BraveWallet.NetworkInfo] = []
  @Published var bitcoinAccounts: [String: BraveWallet.BitcoinAccountInfo] = [:]

  var isObserving: Bool {
    walletServiceObserver != nil && keyringServiceObserver != nil
  }

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    prefilledToken: BraveWallet.BlockchainToken?,
    prefilledAccount: BraveWallet.AccountInfo?,
    userAssetManager: WalletUserAssetManagerType,
    bitcoinWalletService: BraveWalletBitcoinWalletService
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.blockchainRegistry = blockchainRegistry
    self.prefilledToken = prefilledToken
    self.prefilledAccount = prefilledAccount
    self.assetManager = userAssetManager
    self.bitcoinWalletService = bitcoinWalletService

    self.setupObservers()
  }

  func tearDown() {
    walletServiceObserver = nil
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onNetworkListChanged: { [weak self] in
        Task { @MainActor [self] in
          // A network was added or removed, update our network filters for the change.
          guard let rpcService = self?.rpcService else { return }
          self?.networkFilters = await rpcService.allNetworksForSupportedCoins().map { network in
            let existingSelectionValue = self?.networkFilters.first(where: {
              $0.model.chainId == network.chainId
            })?.isSelected
            return .init(isSelected: existingSelectionValue ?? true, model: network)
          }
        }
      }
    )
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _accountsChanged: { [weak self] in
        self?.setup()
      },
      _accountsAdded: { [weak self] _ in
        self?.setup()
      }
    )
  }

  func setup() {
    Task { @MainActor in
      self.allNetworks = await rpcService.allNetworksForSupportedCoins()
      self.networkFilters = allNetworks.map {
        .init(isSelected: true, model: $0)
      }
      if let prefilledAccount, prefilledAccount.coin == .btc {
        self.bitcoinAccounts = await bitcoinWalletService.fetchBitcoinAccountInfo(accounts: [
          prefilledAccount
        ])
      } else {
        self.allAccounts = await keyringService.allAccounts().accounts
        self.bitcoinAccounts = await bitcoinWalletService.fetchBitcoinAccountInfo(
          accounts: allAccounts.filter({ $0.coin == .btc })
        )
      }
      self.allNetworks = await rpcService.allNetworksForSupportedCoins()
      self.update()
    }
  }

  func update() {
    Task { @MainActor in
      let allUserAssets = await assetManager.getAllUserAssetsInNetworkAssets(
        networks: allNetworks,
        includingUserDeleted: false
      )
      // Filter `allTokens` to remove any tokens existing in `allUserAssets`. This is possible for ERC721 tokens in the registry without a `tokenId`, which requires the user to add as a custom token
      let allUserTokens = allUserAssets.flatMap(\.tokens)
      let allBlockchainTokens = await blockchainRegistry.allTokens(in: allNetworks)
        .map { assetsForNetwork in
          NetworkAssets(
            network: assetsForNetwork.network,
            tokens: assetsForNetwork.tokens.filter { token in
              !allUserTokens.contains(where: { $0.id == token.id })
            },
            sortOrder: assetsForNetwork.sortOrder
          )
        }

      let selectedNetworks = networkFilters.filter(\.isSelected).map(\.model)
      let filteredNetworkAssets = (allUserAssets + allBlockchainTokens).filter { networkAssets in
        selectedNetworks.contains {
          $0.chainId.caseInsensitiveCompare(networkAssets.network.chainId) == .orderedSame
        }
      }
      assetViewModels = filteredNetworkAssets.flatMap { networkAssets in
        networkAssets.tokens
          .sorted { $0.symbol < $1.symbol }
          .compactMap { token in
            if !token.isErc721 && !token.isNft {  // no deposit for NFTs
              return DepositTokenViewModel(
                token: token,
                network: networkAssets.network
              )
            }
            return nil
          }
      }
    }
  }

  /// Should be called after dismissing create account. Returns true if an account was created
  @MainActor func handleDismissAddAccount(
    _ tokenViewModel: DepositTokenViewModel
  ) async -> Bool {
    if await keyringService.isAccountAvailable(
      for: tokenViewModel.token.coin,
      chainId: tokenViewModel.network.chainId
    ) {
      self.setup()
      return true
    } else {
      return false
    }
  }
}
