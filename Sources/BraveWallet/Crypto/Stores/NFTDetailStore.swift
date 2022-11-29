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

struct ERC721MetaData: Codable {
  var imageURL: String?
  var name: String?
  var description: String?
  
  enum CodingKeys: String, CodingKey {
    case imageURL = "image"
    case name
    case description
  }
  
  init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)
    if let imageString = try container.decodeIfPresent(String.self, forKey: .imageURL) {
      self.imageURL = imageString.hasPrefix("data:image") ? imageString : imageString.httpifyIpfsUrl
    }
    self.name = try container.decodeIfPresent(String.self, forKey: .name)
    self.description = try container.decodeIfPresent(String.self, forKey: .description)
  }
}

class NFTDetailStore: ObservableObject {
  private let rpcService: BraveWalletJsonRpcService
  let nft: BraveWallet.BlockchainToken
  @Published var isLoading: Bool = false
  @Published var erc721MetaData: ERC721MetaData?
  @Published var networkInfo: BraveWallet.NetworkInfo = .init()
  
  init(
    rpcService: BraveWalletJsonRpcService,
    nft: BraveWallet.BlockchainToken
  ) {
    self.rpcService = rpcService
    self.nft = nft
  }
  
  func fetchMetaData() {
    Task { @MainActor in
      isLoading = true
      
      let allNetworks = await rpcService.allNetworks(nft.coin)
      if let network = allNetworks.first(where: { $0.chainId.caseInsensitiveCompare(nft.chainId) == .orderedSame }) {
        networkInfo = network
      }
      
      let (metaData, _, _) = await rpcService.erc721Metadata(nft.contractAddress, tokenId: nft.tokenId, chainId: nft.chainId)
      
      isLoading = false
      if let data = metaData.data(using: .utf8),
         let result = try? JSONDecoder().decode(ERC721MetaData.self, from: data) {
        erc721MetaData = result
      }
    }
  }
}
