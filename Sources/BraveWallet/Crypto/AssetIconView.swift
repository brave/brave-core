// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import BraveUI

/// Displays an asset's icon from the token registry
///
/// By default, creating an `AssetIconView` will result in a dynamically sized icon based
/// on the users size category. If you for some reason need to obtain a fixed size asset icon,
/// wrap this view in another frame of your desired size, for example:
///
///     AssetIconView(token: .eth)
///       .frame(width: 20, height: 20)
///
struct AssetIconView: View {
  var token: BraveWallet.BlockchainToken
  var network: BraveWallet.NetworkInfo
  /// If we should show the network logo on non-native assets
  var shouldShowNetworkIcon: Bool = false
  @ScaledMetric var length: CGFloat = 40
  var maxLength: CGFloat?
  @ScaledMetric var networkSymbolLength: CGFloat = 15
  var maxNetworkSymbolLength: CGFloat?

  private var fallbackMonogram: some View {
    Blockie(address: token.contractAddress)
      .overlay(
        Text(token.symbol.first?.uppercased() ?? "")
          .font(.system(size: length / 2, weight: .bold, design: .rounded))
          .foregroundColor(.white)
          .shadow(color: .black.opacity(0.3), radius: 2, x: 0, y: 1)
      )
  }

  private var localImage: Image? {
    if network.isNativeAsset(token), let uiImage = network.nativeTokenLogoImage {
      return Image(uiImage: uiImage)
    }
    
    for logo in [token.logo, token.symbol.lowercased()] {
      if let baseURL = BraveWallet.TokenRegistryUtils.tokenLogoBaseURL,
        case let imageURL = baseURL.appendingPathComponent(logo),
        let image = UIImage(contentsOfFile: imageURL.path) {
        return Image(uiImage: image)
      }
    }
    
    return nil
  }

  var body: some View {
    Group {
      if let image = localImage {
        image
          .resizable()
          .aspectRatio(contentMode: .fit)
      } else if let url = URL(string: token.logo) {
        WebImageReader(url: url) { image in
          if let image = image {
            Image(uiImage: image)
              .resizable()
              .aspectRatio(contentMode: .fit)
          } else {
            fallbackMonogram
          }
        }
      } else {
        fallbackMonogram
      }
    }
    .frame(width: min(length, maxLength ?? length), height: min(length, maxLength ?? length))
    .overlay(tokenLogo, alignment: .bottomTrailing)
    .accessibilityHidden(true)
  }
  
  @ViewBuilder private var tokenLogo: some View {
    if shouldShowNetworkIcon,  // explicitly show/not show network logo
       (!network.isNativeAsset(token) || network.nativeTokenLogoName != network.networkLogoName), // non-native asset OR if the network is not the official Ethereum network, but uses ETH as gas
       let image = network.networkLogoImage {
      Image(uiImage: image)
        .resizable()
        .overlay(
          Circle()
            .stroke(lineWidth: 2)
            .foregroundColor(.white)
        )
        .frame(width: min(networkSymbolLength, maxNetworkSymbolLength ?? networkSymbolLength), height: min(networkSymbolLength, maxNetworkSymbolLength ?? networkSymbolLength))
    }
  }
}

#if DEBUG
struct AssetIconView_Previews: PreviewProvider {
  static var previews: some View {
    AssetIconView(token: .previewToken, network: .mockMainnet)
      .previewLayout(.sizeThatFits)
      .padding()
      .previewSizeCategories()
    AssetIconView(
      token: .init(
        contractAddress: "0x55296f69f40ea6d20e478533c15a6b08b654e758",
        name: "XY Oracle",
        logo: "",
        isErc20: true,
        isErc721: false,
        isErc1155: false,
        isNft: false,
        isSpam: false,
        symbol: "XYO",
        decimals: 18,
        visible: false,
        tokenId: "",
        coingeckoId: "",
        chainId: "",
        coin: .eth
      ),
      network: .mockMainnet
    )
    .previewLayout(.sizeThatFits)
    .padding()
    .previewSizeCategories()
  }
}
#endif

struct NFTIconView: View {
  
  /// NFT token
  var token: BraveWallet.BlockchainToken
  /// Network for token
  var network: BraveWallet.NetworkInfo
  /// NFT image url from metadata
  var url: URL?
  /// If we should show the network logo on non-native assets
  var shouldShowNetworkIcon: Bool = false
  
  @ScaledMetric var length: CGFloat = 40
  var maxLength: CGFloat?
  @ScaledMetric var tokenLogoLength: CGFloat = 15
  var maxTokenLogoLength: CGFloat?
  
  @ViewBuilder private var tokenLogo: some View {
    if shouldShowNetworkIcon, let image = network.nativeTokenLogoImage {
      Image(uiImage: image)
        .resizable()
        .overlay(
          Circle()
            .stroke(lineWidth: 2)
            .foregroundColor(.white)
        )
        .frame(
          width: min(tokenLogoLength, maxTokenLogoLength ?? tokenLogoLength),
          height: min(tokenLogoLength, maxTokenLogoLength ?? tokenLogoLength)
        )
    }
  }
  
  var body: some View {
    NFTImageView(urlString: url?.absoluteString ?? "") {
      AssetIconView(
        token: token,
        network: network,
        shouldShowNetworkIcon: shouldShowNetworkIcon,
        length: length
      )
    }
    .cornerRadius(5)
    .frame(
      width: min(length, maxLength ?? length),
      height: min(length, maxLength ?? length)
    )
    .overlay(tokenLogo, alignment: .bottomTrailing)
    .allowsHitTesting(false)
    .accessibilityHidden(true)
  }
}
