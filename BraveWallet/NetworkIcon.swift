// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct NetworkIcon: View {
  
  var network: BraveWallet.NetworkInfo

  @ScaledMetric private var length: CGFloat = 30
  
  var body: some View {
    Group {
      if let (iconName, grayscale) = networkImageInfo {
        Image(iconName, bundle: .current)
          .resizable()
          .aspectRatio(contentMode: .fit)
          .grayscale(grayscale ? 1 : 0)
      } else if let urlString = network.iconUrls.first,
                let url = URL(string: urlString) {
        WebImageReader(url: url) { image, isFinished in
          if let image = image {
            Image(uiImage: image)
              .aspectRatio(contentMode: .fit)
              .clipShape(Circle())
          } else {
            networkIconMonogram
          }
        }
      } else {
        networkIconMonogram
      }
    }
    .aspectRatio(1, contentMode: .fit)
    .frame(width: length, height: length)
  }
  
  private var networkIconMonogram: some View {
    Blockie(address: network.chainName)
      .overlay(
        Text(network.chainName.first?.uppercased() ?? "")
          .font(.system(size: length / 2, weight: .bold, design: .rounded))
          .foregroundColor(.white)
          .shadow(color: .black.opacity(0.3), radius: 2, x: 0, y: 1)
      )
  }
  
  private typealias NetworkImageInfo = (iconName: String, grayscale: Bool)
  private var networkImageInfo: NetworkImageInfo? {
    switch network.chainId {
    case BraveWallet.MainnetChainId:
      return ("eth-asset-icon", false)
    case BraveWallet.RinkebyChainId,
      BraveWallet.RopstenChainId,
      BraveWallet.GoerliChainId,
      BraveWallet.KovanChainId:
      return ("eth-asset-icon", true)
    case BraveWallet.SolanaMainnet:
      return ("sol-asset-icon", false)
    case BraveWallet.SolanaTestnet, BraveWallet.SolanaDevnet:
      return ("sol-asset-icon", true)
    case BraveWallet.FilecoinMainnet:
      return ("filecoin-asset-icon", false)
    case BraveWallet.FilecoinTestnet:
      return ("filecoin-asset-icon", true)
    case BraveWallet.PolygonMainnetChainId:
      return ("matic", false)
    case BraveWallet.BinanceSmartChainMainnetChainId:
      return ("bnb-asset-icon", false)
    case BraveWallet.CeloMainnetChainId:
      return ("celo", false)
    case BraveWallet.AvalancheMainnetChainId:
      return ("avax", false)
    case BraveWallet.FantomMainnetChainId:
      return ("fantom", false)
    case BraveWallet.OptimismMainnetChainId:
      return ("optimism", false)
    default:
      return nil
    }
  }
}
