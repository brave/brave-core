// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import SwiftUI

struct NFTsGridView: View {

  let assets: [NFTAssetViewModel]
  let selectedAsset: (BraveWallet.BlockchainToken) -> Void

  private let nftGrids = [GridItem(.adaptive(minimum: 120), spacing: 16, alignment: .top)]

  var body: some View {
    ScrollView {
      if assets.isEmpty {
        emptyView
      } else {
        nftGrid
      }
    }
    .background(Color(braveSystemName: .containerBackground))
  }

  private var emptyView: some View {
    VStack(alignment: .center, spacing: 10) {
      Text(Strings.Wallet.nftPageEmptyTitle)
        .font(.headline.weight(.semibold))
        .foregroundColor(Color(.braveLabel))
    }
    .multilineTextAlignment(.center)
    .frame(maxWidth: .infinity)
    .padding(.vertical, 60)
    .padding(.horizontal, 32)
  }

  private var nftGrid: some View {
    LazyVGrid(columns: nftGrids) {
      ForEach(assets) { nft in
        Button {
          selectedAsset(nft.token)
        } label: {
          VStack(alignment: .leading, spacing: 4) {
            nftImage(nft)
              .overlay(alignment: .bottomTrailing) {
                nftLogo(nft)
                  .offset(y: 12)
              }
              .padding(.bottom, 8)
            Text(nft.token.nftTokenTitle)
              .font(.callout.weight(.medium))
              .foregroundColor(Color(.braveLabel))
              .multilineTextAlignment(.leading)
            if !nft.token.symbol.isEmpty {
              Text(nft.token.symbol)
                .font(.caption)
                .foregroundColor(Color(.secondaryBraveLabel))
                .multilineTextAlignment(.leading)
            }
          }
          .overlay(alignment: .topLeading) {
            if nft.token.isSpam {
              HStack(spacing: 4) {
                Text(Strings.Wallet.nftSpam)
                  .padding(.vertical, 4)
                  .padding(.leading, 6)
                  .foregroundColor(Color(.braveErrorLabel))
                Image(braveSystemName: "leo.warning.triangle-outline")
                  .padding(.vertical, 4)
                  .padding(.trailing, 6)
                  .foregroundColor(Color(.braveErrorBorder))
              }
              .font(.system(size: 13).weight(.semibold))
              .background(
                Color(uiColor: WalletV2Design.spamNFTLabelBackground)
                  .cornerRadius(4)
              )
              .padding(12)
            }
          }
        }
      }
    }
    .padding()
  }

  @ViewBuilder private func nftImage(_ nftViewModel: NFTAssetViewModel) -> some View {
    Group {
      if let urlString = nftViewModel.nftMetadata?.image {
        NFTImageView(urlString: urlString) {
          LoadingNFTView(shimmer: false)
        }
      } else {
        LoadingNFTView(shimmer: false)
      }
    }
    .cornerRadius(4)
  }

  @ViewBuilder private func nftLogo(_ nftViewModel: NFTAssetViewModel) -> some View {
    if let image = nftViewModel.network.nativeTokenLogoImage,
      Preferences.Wallet.isShowingNFTNetworkLogoFilter.value
    {
      Image(uiImage: image)
        .resizable()
        .overlay {
          Circle()
            .stroke(lineWidth: 2)
            .foregroundColor(Color(braveSystemName: .containerBackground))
        }
        .frame(width: 24, height: 24)
    }
  }
}
