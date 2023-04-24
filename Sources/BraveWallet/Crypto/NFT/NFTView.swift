// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct NFTView: View {
  var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var nftStore: NFTStore
  
  @State private var isPresentingNetworkFilter: Bool = false
  @State private var isPresentingEditUserAssets: Bool = false
  @State private var selectedNFTViewModel: NFTAssetViewModel?
  
  @Environment(\.buySendSwapDestination)
  private var buySendSwapDestination: Binding<BuySendSwapDestination?>
  
  private var emptyView: some View {
    VStack(alignment: .center, spacing: 10) {
      Text(Strings.Wallet.nftPageEmptyTitle)
        .font(.headline.weight(.semibold))
        .foregroundColor(Color(.braveLabel))
      Text(Strings.Wallet.nftPageEmptyDescription)
        .font(.subheadline.weight(.semibold))
        .foregroundColor(Color(.secondaryLabel))
    }
    .multilineTextAlignment(.center)
    .frame(maxWidth: .infinity)
    .padding(.vertical, 60)
    .padding(.horizontal, 32)
  }
  
  private var editUserAssetsButton: some View {
    Button(action: { isPresentingEditUserAssets = true }) {
      Text(Strings.Wallet.editVisibleAssetsButtonTitle)
        .multilineTextAlignment(.center)
        .font(.footnote.weight(.semibold))
        .foregroundColor(Color(.braveBlurple))
        .frame(maxWidth: .infinity)
    }
    .sheet(isPresented: $isPresentingEditUserAssets) {
      EditUserAssetsView(
        networkStore: networkStore,
        keyringStore: keyringStore,
        userAssetsStore: nftStore.userAssetsStore
      ) {
        cryptoStore.updateAssets()
      }
    }
  }
  
  private let nftGrids = [GridItem(.adaptive(minimum: 120), spacing: 16, alignment: .top)]
  
  @ViewBuilder private func nftLogo(_ nftViewModel: NFTAssetViewModel) -> some View {
    if let image = nftViewModel.network.nativeTokenLogoImage {
      Image(uiImage: image)
        .resizable()
        .frame(width: 20, height: 20)
        .padding(4)
    }
  }
  
  @ViewBuilder private func nftImage(_ nftViewModel: NFTAssetViewModel) -> some View {
    Group {
      if let urlString = nftViewModel.nftMetadata?.imageURLString {
        NFTImageView(urlString: urlString) {
          noImageView(nftViewModel)
        }
      } else {
        noImageView(nftViewModel)
      }
    }
    .overlay(nftLogo(nftViewModel), alignment: .bottomTrailing)
    .cornerRadius(4)
  }
  
  @ViewBuilder private func noImageView(_ nftViewModel: NFTAssetViewModel) -> some View {
    Blockie(address: nftViewModel.token.contractAddress, shape: .rectangle)
      .overlay(
        Text(nftViewModel.token.symbol.first?.uppercased() ?? "")
          .font(.system(size: 80, weight: .bold, design: .rounded))
          .foregroundColor(.white)
          .shadow(color: .black.opacity(0.3), radius: 2, x: 0, y: 1)
      )
      .aspectRatio(1.0, contentMode: .fit)
  }
  
  private var networkFilterButton: some View {
    Button(action: {
      self.isPresentingNetworkFilter = true
    }) {
      HStack {
        Text(nftStore.networkFilter.title)
        Image(braveSystemName: "brave.text.alignleft")
      }
      .font(.footnote.weight(.medium))
      .foregroundColor(Color(.braveBlurpleTint))
    }
    .sheet(isPresented: $isPresentingNetworkFilter) {
      NavigationView {
        NetworkFilterView(
          networkFilter: $nftStore.networkFilter,
          networkStore: networkStore
        )
      }
      .onDisappear {
        networkStore.closeNetworkSelectionStore()
      }
    }
  }
  
  private var nftHeaderView: some View {
    HStack {
      Text(Strings.Wallet.assetsTitle)
        .font(.caption)
        .foregroundColor(Color(.secondaryBraveLabel))
      Spacer()
      networkFilterButton
    }
    .textCase(nil)
    .padding(.horizontal, 10)
    .frame(maxWidth: .infinity, alignment: .leading)
  }
  
  var body: some View {
    ScrollView {
      VStack {
        nftHeaderView
        if nftStore.userVisibleNFTs.isEmpty {
          emptyView
            .listRowBackground(Color(.clear))
        } else {
          LazyVGrid(columns: nftGrids) {
            ForEach(nftStore.userVisibleNFTs) { nft in
              Button(action: {
                selectedNFTViewModel = nft
              }) {
                VStack(alignment: .leading, spacing: 4) {
                  nftImage(nft)
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
              }
            }
          }
        }
        VStack(spacing: 16) {
          Divider()
          editUserAssetsButton
        }
        .padding(.top, 20)
      }
      .padding(24)
    }
    .background(
      NavigationLink(
        isActive: Binding(
          get: { selectedNFTViewModel != nil },
          set: { if !$0 { selectedNFTViewModel = nil } }
        ),
        destination: {
          if let nftViewModel = selectedNFTViewModel {
            NFTDetailView(
              nftDetailStore: cryptoStore.nftDetailStore(for: nftViewModel.token, nftMetadata: nftViewModel.nftMetadata),
              buySendSwapDestination: buySendSwapDestination
            ) { nftMetadata in
              nftStore.updateNFTMetadataCache(for: nftViewModel.token, metadata: nftMetadata)
            }
            .onDisappear {
              cryptoStore.closeNFTDetailStore(for: nftViewModel.token)
            }
          }
        },
        label: {
          EmptyView()
        })
    )
    .background(Color(UIColor.braveGroupedBackground))
  }
}

#if DEBUG
struct NFTView_Previews: PreviewProvider {
  static var previews: some View {
    NFTView(
      cryptoStore: .previewStore,
      keyringStore: .previewStore,
      networkStore: .previewStore,
      nftStore: CryptoStore.previewStore.nftStore
    )
  }
}
#endif
