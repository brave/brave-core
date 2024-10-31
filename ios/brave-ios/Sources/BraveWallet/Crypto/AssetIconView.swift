// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import SwiftUI

/// Displays an asset's icon from the token registry or logo.
struct AssetIcon: View {
  let token: BraveWallet.BlockchainToken
  let network: BraveWallet.NetworkInfo?

  var body: some View {
    Group {
      if let uiImage = token.localImage(network: network) {
        Image(uiImage: uiImage)
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
  }

  @State private var monogramSize: CGSize = .zero
  private var fallbackMonogram: some View {
    BlockieBackground(
      seed: token.contractAddress.isEmpty ? token.name : token.contractAddress.lowercased()
    )
    .clipShape(Circle())
    .readSize(onChange: { newSize in
      monogramSize = newSize
    })
    .overlay(
      Text(token.symbol.first?.uppercased() ?? "")
        .font(
          .system(
            size: max(monogramSize.width, monogramSize.height) / 2,
            weight: .bold,
            design: .rounded
          )
        )
        .foregroundColor(.white)
        .shadow(color: .black.opacity(0.3), radius: 2, x: 0, y: 1)
    )
  }
}

/// Displays an asset's icon from the token registry or logo.
///
/// By default, creating an `AssetIconView` will result in a dynamically sized icon based
/// on the users size category.
struct AssetIconView: View {
  var token: BraveWallet.BlockchainToken
  var network: BraveWallet.NetworkInfo?
  /// If we should show the network logo on non-native assets. NetworkInfo is required.
  var shouldShowNetworkIcon: Bool = false
  @ScaledMetric var length: CGFloat = 40
  var maxLength: CGFloat?
  @ScaledMetric var networkSymbolLength: CGFloat = 15
  var maxNetworkSymbolLength: CGFloat?

  var body: some View {
    AssetIcon(token: token, network: network)
      .frame(width: min(length, maxLength ?? length), height: min(length, maxLength ?? length))
      .overlay(tokenNetworkLogo, alignment: .bottomTrailing)
      .accessibilityHidden(true)
  }

  @ViewBuilder private var tokenNetworkLogo: some View {
    if let network,
      shouldShowNetworkIcon,  // explicitly show/not show network logo
      // non-native asset OR if the network is not the official Ethereum network, but uses ETH as gas
      !network.isNativeAsset(token) || network.nativeTokenLogoName != network.networkLogoName,
      let image = network.networkLogoImage
    {
      Image(uiImage: image)
        .resizable()
        .overlay(
          Circle()
            .stroke(lineWidth: 2)
            .foregroundColor(Color(braveSystemName: .containerBackground))
        )
        .frame(
          width: min(networkSymbolLength, maxNetworkSymbolLength ?? networkSymbolLength),
          height: min(networkSymbolLength, maxNetworkSymbolLength ?? networkSymbolLength)
        )
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
        isCompressed: false,
        isErc20: true,
        isErc721: false,
        isErc1155: false,
        splTokenProgram: .unsupported,
        isNft: false,
        isSpam: false,
        symbol: "XYO",
        decimals: 18,
        visible: false,
        tokenId: "",
        coingeckoId: "",
        chainId: "",
        coin: .eth,
        isShielded: false
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
  /// View is loading NFT metadata
  var isLoadingMetadata: Bool = false

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
            .foregroundColor(Color(braveSystemName: .containerBackground))
        )
        .frame(
          width: min(tokenLogoLength, maxTokenLogoLength ?? tokenLogoLength),
          height: min(tokenLogoLength, maxTokenLogoLength ?? tokenLogoLength)
        )
    }
  }

  var body: some View {
    NFTImageView(  // logo populated for auto-discovered NFTs
      urlString: url?.absoluteString ?? token.logo,
      isLoading: isLoadingMetadata
    ) {
      LoadingNFTView(shimmer: false)
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

/// Displays 2 asset icons stacked on top of one another, with the token's network logo on top.
/// `bottomToken` is displayed in the top-right, `topToken` is displayed in the bottom-left,
/// and the network logo is displayed in the bottom-right.
struct StackedAssetIconsView: View {

  var bottomToken: BraveWallet.BlockchainToken?
  var topToken: BraveWallet.BlockchainToken?
  var network: BraveWallet.NetworkInfo

  /// Length of entire view
  @ScaledMetric var length: CGFloat = 40
  /// Max length of entire view
  var maxLength: CGFloat?
  @ScaledMetric var networkSymbolLength: CGFloat = 15
  var maxNetworkSymbolLength: CGFloat?

  /// Size of padding applied to bottom/top icon so they can overlap
  private var iconPadding: CGFloat {
    length / 5
  }

  /// Size of asset icon
  private var assetIconLength: CGFloat {
    length - iconPadding
  }

  /// Max size of asset icon
  private var maxAssetIconLength: CGFloat? {
    guard let maxLength else { return nil }
    return maxLength - iconPadding
  }

  var body: some View {
    ZStack {
      Group {
        if let bottomToken {
          AssetIconView(
            token: bottomToken,
            network: network,
            shouldShowNetworkIcon: false,
            length: assetIconLength,
            maxLength: maxAssetIconLength
          )
        } else {  // nil token possible for Solana Swaps
          GenericAssetIconView(
            backgroundColor: Color(braveSystemName: .neutral40),
            iconColor: Color.white,
            length: assetIconLength,
            maxLength: maxAssetIconLength
          )
        }
      }
      .padding(.leading, iconPadding)
      .padding(.bottom, iconPadding)
      .zIndex(0)

      Group {
        if let topToken {
          AssetIconView(
            token: topToken,
            network: network,
            shouldShowNetworkIcon: false,
            length: assetIconLength,
            maxLength: maxAssetIconLength
          )
        } else {  // nil token possible for Solana Swaps
          GenericAssetIconView(
            backgroundColor: Color(braveSystemName: .neutral20),
            iconColor: Color.black,
            length: assetIconLength,
            maxLength: maxAssetIconLength
          )
        }
      }
      .padding(.top, iconPadding)
      .padding(.trailing, iconPadding)
      .zIndex(1)

      if let networkLogoImage = network.networkLogoImage {
        Group {
          Image(uiImage: networkLogoImage)
            .resizable()
            .overlay(
              Circle()
                .stroke(lineWidth: 2)
                .foregroundColor(Color(braveSystemName: .containerBackground))
            )
            .frame(
              width: min(networkSymbolLength, maxNetworkSymbolLength ?? networkSymbolLength),
              height: min(networkSymbolLength, maxNetworkSymbolLength ?? networkSymbolLength)
            )
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .bottomTrailing)
        .zIndex(2)
      }
    }
    .frame(
      width: min(length, maxLength ?? length),
      height: min(length, maxLength ?? length)
    )
  }
}

#if DEBUG
struct StackedAssetIconsView_Previews: PreviewProvider {
  static var previews: some View {
    StackedAssetIconsView(
      bottomToken: nil,
      topToken: nil,
      network: .mockSolana
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif

struct GenericAssetIconView: View {

  let backgroundColor: Color
  let iconColor: Color
  @ScaledMetric var length: CGFloat
  let maxLength: CGFloat?

  init(
    backgroundColor: Color = Color(braveSystemName: .neutral20),
    iconColor: Color = Color.black,
    length: CGFloat = 40,
    maxLength: CGFloat? = nil
  ) {
    self.backgroundColor = backgroundColor
    self.iconColor = iconColor
    self._length = .init(wrappedValue: length)
    self.maxLength = maxLength
  }

  var body: some View {
    Circle()
      .fill(backgroundColor)
      .frame(width: min(length, maxLength ?? length), height: min(length, maxLength ?? length))
      .overlay {
        Image(braveSystemName: "leo.crypto.wallets")
          .resizable()
          .aspectRatio(contentMode: .fit)
          .padding(6)
          .foregroundColor(iconColor)
      }
  }
}
