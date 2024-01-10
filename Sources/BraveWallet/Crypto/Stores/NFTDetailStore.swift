/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore

struct NFTMetadata: Codable, Equatable {
  var imageURLString: String?
  var name: String?
  var description: String?
  var attributes: [NFTAttribute]?

  enum CodingKeys: String, CodingKey {
    case imageURLString = "image"
    case name
    case description
    case attributes
  }
  
  init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)
    self.imageURLString = try container.decodeIfPresent(String.self, forKey: .imageURLString)
    self.name = try container.decodeIfPresent(String.self, forKey: .name)
    self.description = try container.decodeIfPresent(String.self, forKey: .description)
    let test = try container.decodeIfPresent([NFTAttribute].self, forKey: .attributes)
    self.attributes = test
  }
  
  init(
    imageURLString: String?,
    name: String?,
    description: String?,
    attributes: [NFTAttribute]?
  ) {
    self.imageURLString = imageURLString
    self.name = name
    self.description = description
    self.attributes = attributes
  }

  func httpfyIpfsUrl(ipfsApi: IpfsAPI) -> NFTMetadata {
    guard let imageURLString,
          imageURLString.hasPrefix("ipfs://"),
          let url = URL(string: imageURLString) else {
      return NFTMetadata(imageURLString: self.imageURLString, name: self.name, description: self.description, attributes: self.attributes)
    }
    return NFTMetadata(imageURLString: ipfsApi.resolveGatewayUrl(for: url)?.absoluteString, name: self.name, description: self.description, attributes: self.attributes)
  }
  
  var imageURL: URL? {
    guard let urlString = imageURLString else { return nil }
    return URL(string: urlString)
  }
}

struct NFTAttribute: Codable, Equatable, Identifiable {
  var type: String
  var value: String
  var id: String { type }
  
  enum CodingKeys: String, CodingKey {
    case type = "trait_type"
    case value
  }
}

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
  @Published var nftMetadata: NFTMetadata?
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
    nftMetadata: NFTMetadata?,
    owner: BraveWallet.AccountInfo?
  ) {
    self.assetManager = assetManager
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.txService = txService
    self.ipfsApi = ipfsApi
    self.nft = nft
    self.nftMetadata = nftMetadata?.httpfyIpfsUrl(ipfsApi: ipfsApi)
    self.owner = owner
    
    self.setupObservers()
    
    self.update()
  }
  
  func setupObservers() {
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onTransactionStatusChanged: { [weak self] txInfo in
        if txInfo.txStatus == .confirmed, txInfo.isSend, (txInfo.coin == .eth || txInfo.coin == .sol) {
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
      let allNetworks = await rpcService.allNetworks(nft.coin)
      if let network = allNetworks.first(where: { $0.chainId.caseInsensitiveCompare(nft.chainId) == .orderedSame }) {
        networkInfo = network
      }
      
      if owner == nil {
        updateOwner()
      }
      
      if nftMetadata == nil {
        isLoading = true
        nftMetadata = await rpcService.fetchNFTMetadata(for: nft, ipfsApi: self.ipfsApi)
        isLoading = false
      }
    }
  }
  
  func updateNFTStatus(
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool,
    completion: @escaping () -> Void
  ) {
    assetManager.updateUserAsset(
      for: nft,
      visible: visible,
      isSpam: isSpam,
      isDeletedByUser: isDeletedByUser
    ) { [weak self] in
      guard let self else { return }
      if let newNFT = self.assetManager.getUserAsset(self.nft)?.blockchainToken {
        self.nft = newNFT
      }
      completion()
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
        body: { @MainActor [rpcService, nft] group in
          for account in accounts where account.coin == nft.coin {
            group.addTask { @MainActor in
              let balanceForToken = await rpcService.balance(
                for: nft,
                in: account,
                network: network
              )
              return [account.address: Int(balanceForToken ?? 0)]
            }
          }
          return await group.reduce(into: [String: Int](), { partialResult, new in
            for key in new.keys {
              partialResult[key] = new[key]
            }
          })
        }
      )
      if let address = nftBalances.first(where: { address, balance in
        balance > 0
      })?.key,
         let account = accounts.first(where: { accountInfo in
           accountInfo.address.caseInsensitiveCompare(address) == .orderedSame
         }) {
        owner = account
      } else {
        owner = nil
      }
    }
  }
}
