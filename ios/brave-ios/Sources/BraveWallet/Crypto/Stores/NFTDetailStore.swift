// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

class NFTDetailStore: ObservableObject, WalletObserverStore {
  private let assetManager: WalletUserAssetManagerType
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletTxService
  private let ipfsApi: IpfsAPI
  private var txServiceObserver: TxServiceObserver?
  @Published var owner: BraveWallet.AccountInfo?
  @Published var nft: BraveWallet.BlockchainToken
  @Published var isLoading: Bool = false
  @Published var nftMetadata: BraveWallet.NftMetadata?
  @Published var networkInfo: BraveWallet.NetworkInfo?

  var isObserving: Bool {
    txServiceObserver != nil
  }

  init(
    assetManager: WalletUserAssetManagerType,
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    txService: BraveWalletTxService,
    ipfsApi: IpfsAPI,
    nft: BraveWallet.BlockchainToken,
    nftMetadata: BraveWallet.NftMetadata?,
    owner: BraveWallet.AccountInfo?
  ) {
    self.assetManager = assetManager
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.txService = txService
    self.ipfsApi = ipfsApi
    self.nft = nft
    self.nftMetadata = nftMetadata
    self.owner = owner

    self.setupObservers()

    self.update()
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.assetManager.addUserAssetDataObserver(self)
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onTransactionStatusChanged: { [weak self] txInfo in
        if txInfo.txStatus == .confirmed, txInfo.isSend, txInfo.coin == .eth || txInfo.coin == .sol
        {
          self?.updateOwner()
        }
      }
    )
  }

  func tearDown() {
    txServiceObserver = nil
  }

  func update() {
    Task { @MainActor in
      let allNetworks = await rpcService.allNetworks()
      if let network = allNetworks.first(where: {
        $0.coin == nft.coin && $0.chainId.caseInsensitiveCompare(nft.chainId) == .orderedSame
      }) {
        networkInfo = network
      }

      updateOwner()

      if nftMetadata == nil {
        isLoading = true
        nftMetadata = await rpcService.fetchNFTMetadata(
          for: nft,
          ipfsApi: ipfsApi
        )
        isLoading = false
      }
    }
  }

  @MainActor func updateNFTStatus(
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool
  ) async {
    await assetManager.updateUserAsset(
      for: nft,
      visible: visible,
      isSpam: isSpam,
      isDeletedByUser: isDeletedByUser
    )
    if let newNFT = assetManager.getUserAsset(nft)?.blockchainToken {
      nft = newNFT
    }
  }

  var updateOwnerTask: Task<(), Never>?
  private func updateOwner() {
    updateOwnerTask?.cancel()
    updateOwnerTask = Task { @MainActor in
      guard let network = networkInfo else { return }
      let accounts = await keyringService.allAccounts().accounts
      let nftBalances: [String: Int] = await withTaskGroup(
        of: [String: Int].self,
        body: { @MainActor [assetManager, rpcService, nft] group in
          for account in accounts where account.coin == nft.coin {
            group.addTask { @MainActor in
              if let assetBalance = assetManager.getAssetBalances(
                for: nft,
                account: account.id
              )?.first {
                return [account.id: (assetBalance.balance as NSString).integerValue]
              } else {
                let balanceForToken = await rpcService.balance(
                  for: nft,
                  in: account,
                  network: network
                )
                return [account.id: Int(balanceForToken ?? 0)]
              }
            }
          }
          return await group.reduce(
            into: [String: Int](),
            { partialResult, new in
              for key in new.keys {
                partialResult[key] = new[key]
              }
            }
          )
        }
      )
      if let uniqueKey = nftBalances.first(where: { _, balance in
        balance > 0
      })?.key,
        let account = accounts.first(where: { accountInfo in
          accountInfo.id.caseInsensitiveCompare(uniqueKey) == .orderedSame
        })
      {
        owner = account
      } else {
        owner = nil
      }
    }
  }
}

extension NFTDetailStore: WalletUserAssetDataObserver {
  func cachedBalanceRefreshed() {
    update()
  }

  func userAssetUpdated() {
  }
}
