/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore

private extension String {
  var httpifyIpfsUrl: String {
    let trimmedUrl = self.trim(" ")
    return trimmedUrl.hasPrefix("ipfs://") ? trimmedUrl.replacingOccurrences(of: "ipfs://", with: "https://ipfs.io/ipfs/") : trimmedUrl
  }
}

struct NFTMetadata: Codable, Equatable {
  var imageURLString: String?
  var name: String?
  var description: String?
  
  enum CodingKeys: String, CodingKey {
    case imageURLString = "image"
    case name
    case description
  }
  
  init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)
    if let imageString = try container.decodeIfPresent(String.self, forKey: .imageURLString) {
      self.imageURLString = imageString.hasPrefix("data:image") ? imageString : imageString.httpifyIpfsUrl
    }
    self.name = try container.decodeIfPresent(String.self, forKey: .name)
    self.description = try container.decodeIfPresent(String.self, forKey: .description)
  }
  
  init(
    imageURLString: String?,
    name: String?,
    description: String?
  ) {
    self.imageURLString = imageURLString
    self.name = name
    self.description = description
  }
  
  var imageURL: URL? {
    guard let urlString = imageURLString else { return nil }
    return URL(string: urlString)
  }
}

class NFTDetailStore: ObservableObject {
  private let rpcService: BraveWalletJsonRpcService
  let nft: BraveWallet.BlockchainToken
  @Published var isLoading: Bool = false
  @Published var nftMetadata: NFTMetadata?
  @Published var networkInfo: BraveWallet.NetworkInfo = .init()
  
  init(
    rpcService: BraveWalletJsonRpcService,
    nft: BraveWallet.BlockchainToken,
    nftMetadata: NFTMetadata?
  ) {
    self.rpcService = rpcService
    self.nft = nft
    self.nftMetadata = nftMetadata
  }
  
  func update() {
    Task { @MainActor in
      let allNetworks = await rpcService.allNetworks(nft.coin)
      if let network = allNetworks.first(where: { $0.chainId.caseInsensitiveCompare(nft.chainId) == .orderedSame }) {
        networkInfo = network
      }
      
      if nftMetadata == nil {
        isLoading = true
        nftMetadata = await rpcService.fetchNFTMetadata(for: nft)
        isLoading = false
      }
    }
  }
}
