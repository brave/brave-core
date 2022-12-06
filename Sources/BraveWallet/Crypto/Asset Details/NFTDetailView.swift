/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveCore
import DesignSystem
import BraveUI
import SDWebImageSwiftUI

struct NFTDetailView: View {
  @ObservedObject var nftDetailStore: NFTDetailStore
  @Binding var buySendSwapDestination: BuySendSwapDestination?
  
  @Environment(\.openWalletURLAction) private var openWalletURL
  
  @ViewBuilder private var noImageView: some View {
    Text(Strings.Wallet.nftDetailImageNotAvailable)
      .foregroundColor(Color(.secondaryBraveLabel))
      .frame(maxWidth: .infinity, minHeight: 300)
  }
  
  @ViewBuilder private var nftImage: some View {
    if let erc721Metadata = nftDetailStore.erc721Metadata {
      if let urlString = erc721Metadata.imageURLString {
        NFTImageView(urlString: urlString) {
          noImageView
        }
        .cornerRadius(10)
      } else {
        noImageView
      }
    } else {
      noImageView
    }
  }
  
  private var isSVGImage: Bool {
    guard let erc721Metadata = nftDetailStore.erc721Metadata, let imageUrlString = erc721Metadata.imageURLString else { return false }
    return imageUrlString.hasPrefix("data:image/svg") || imageUrlString.hasSuffix(".svg")
  }
  
  var body: some View {
    ScrollView(.vertical) {
      VStack(alignment: .leading, spacing: 24) {
        VStack(spacing: 8) {
          if nftDetailStore.isLoading {
            ProgressView()
              .frame(maxWidth: .infinity, minHeight: 300)
          } else {
            nftImage
          }
          VStack(alignment: .leading, spacing: 8) {
            Text(nftDetailStore.nft.nftTokenTitle)
              .font(.title3.weight(.semibold))
              .foregroundColor(Color(.braveLabel))
            Text(nftDetailStore.nft.name)
              .foregroundColor(Color(.secondaryBraveLabel))
            Button(action: {
              buySendSwapDestination = BuySendSwapDestination(
                kind: .send,
                initialToken: nftDetailStore.nft
              )
            }) {
              Text(Strings.Wallet.nftDetailSendNFTButtonTitle)
                .frame(maxWidth: .infinity)
            }
            .buttonStyle(BraveFilledButtonStyle(size: .large))
          }
        }
        if let erc721Metadata = nftDetailStore.erc721Metadata, let description = erc721Metadata.description {
          VStack(alignment: .leading, spacing: 8) {
            Text(Strings.Wallet.nftDetailDescription)
              .font(.headline.weight(.semibold))
              .foregroundColor(Color(.braveLabel))
            Text(description)
              .foregroundColor(Color(.braveLabel))
          }
        }
        VStack(spacing: 16) {
          Group {
            HStack {
              Text(Strings.Wallet.nftDetailBlockchain)
                .font(.headline.weight(.semibold))
              Spacer()
              Text(nftDetailStore.networkInfo.chainName)
            }
            HStack {
              Text(Strings.Wallet.nftDetailTokenStandard)
                .font(.headline.weight(.semibold))
              Spacer()
              Text(Strings.Wallet.nftDetailERC721)
            }
            HStack {
              Text(Strings.Wallet.nftDetailTokenID)
                .font(.headline.weight(.semibold))
              Spacer()
              Button(action: {
                if let explorerURL = nftDetailStore.networkInfo.blockExplorerUrls.first {
                  let baseURL = "\(explorerURL)/token/\(nftDetailStore.nft.contractAddress)"
                  var nftURL = URL(string: baseURL)
                  if let tokenId = Int(nftDetailStore.nft.tokenId.removingHexPrefix, radix: 16) {
                    nftURL = URL(string: "\(baseURL)?a=\(tokenId)")
                  }
                  
                  if let url = nftURL {
                    openWalletURL?(url)
                  }
                }
              }) {
                if let tokenId = Int(nftDetailStore.nft.tokenId.removingHexPrefix, radix: 16) {
                  Text(verbatim: "#\(tokenId)")
                    .foregroundColor(Color(.braveBlurple))
                } else {
                  Text("\(nftDetailStore.nft.name) #\(nftDetailStore.nft.tokenId)")
                    .foregroundColor(Color(.braveBlurple))
                }
              }
            }
          }
          .foregroundColor(Color(.braveLabel))
        }
        if isSVGImage {
          Text(Strings.Wallet.nftDetailSVGImageDisclaimer)
            .multilineTextAlignment(.center)
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
            .frame(maxWidth: .infinity)
        }
      }
      .padding()
    }
    .onAppear {
      if nftDetailStore.erc721Metadata == nil {
        nftDetailStore.fetchMetadata()
      }
    }
    .background(Color(UIColor.braveGroupedBackground).ignoresSafeArea())
    .navigationBarTitle(Strings.Wallet.nftDetailTitle)
  }
}
