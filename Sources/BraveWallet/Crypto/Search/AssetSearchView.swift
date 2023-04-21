/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import Strings

struct AssetSearchView: View {
  var keyringStore: KeyringStore
  var cryptoStore: CryptoStore
  var userAssetsStore: UserAssetsStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var allAssets: [AssetViewModel] = []
  @State private var allNFTMetadata: [String: NFTMetadata] = [:]
  @State private var query = ""
  @State private var networkFilter: NetworkFilter = .allNetworks
  @State private var isPresentingNetworkFilter = false

  private var filteredTokens: [AssetViewModel] {
    let allAssets: [AssetViewModel]
    switch networkFilter {
    case .allNetworks:
      allAssets = self.allAssets
    case .network(let network):
      allAssets = self.allAssets.filter { $0.network.chainId == network.chainId }
    }
    let normalizedQuery = query.lowercased()
    if normalizedQuery.isEmpty {
      return allAssets
    }
    return allAssets.filter {
      $0.token.symbol.lowercased().contains(normalizedQuery) || $0.token.name.lowercased().contains(normalizedQuery)
    }
  }
  
  private var networkFilterButton: some View {
    Button(action: {
      self.isPresentingNetworkFilter = true
    }) {
      HStack {
        Image(braveSystemName: "leo.list")
        Text(networkFilter.title)
      }
      .font(.footnote.weight(.medium))
      .foregroundColor(Color(.braveBlurpleTint))
    }
    .sheet(isPresented: $isPresentingNetworkFilter) {
      NavigationView {
        NetworkFilterView(
          networkFilter: $networkFilter,
          networkStore: cryptoStore.networkStore
        )
      }
      .onDisappear {
        cryptoStore.networkStore.closeNetworkSelectionStore()
      }
    }
  }
  
  var body: some View {
    NavigationView {
      List {
        Section(
          header: WalletListHeaderView(
            title: Text(Strings.Wallet.assetsTitle)
          )
        ) {
          Group {
            if filteredTokens.isEmpty {
              Text(Strings.Wallet.assetSearchEmpty)
                .font(.footnote)
                .foregroundColor(Color(.secondaryBraveLabel))
                .multilineTextAlignment(.center)
                .frame(maxWidth: .infinity)
            } else {
              ForEach(filteredTokens) { assetViewModel in
                
                NavigationLink(
                  destination: {
                    if assetViewModel.token.isErc721 {
                      NFTDetailView(
                        nftDetailStore: cryptoStore.nftDetailStore(for: assetViewModel.token, nftMetadata: allNFTMetadata[assetViewModel.token.id]),
                        buySendSwapDestination: .constant(nil)
                      ) { metadata in
                        allNFTMetadata[assetViewModel.token.id] = metadata
                      }
                      .onDisappear {
                        cryptoStore.closeNFTDetailStore(for: assetViewModel.token)
                      }
                    } else {
                      AssetDetailView(
                        assetDetailStore: cryptoStore.assetDetailStore(for: assetViewModel.token),
                        keyringStore: keyringStore,
                        networkStore: cryptoStore.networkStore
                      )
                      .onDisappear {
                        cryptoStore.closeAssetDetailStore(for: assetViewModel.token)
                      }
                    }
                  }
                ) {
                  SearchAssetView(
                    title: title(for: assetViewModel.token),
                    symbol: assetViewModel.token.symbol,
                    networkName: assetViewModel.network.chainName
                  ) {
                    if assetViewModel.token.isErc721 || assetViewModel.token.isNft {
                      NFTIconView(
                        token: assetViewModel.token,
                        network: assetViewModel.network,
                        url: allNFTMetadata[assetViewModel.token.id]?.imageURL,
                        shouldShowNativeTokenIcon: true
                      )
                    } else {
                      AssetIconView(
                        token: assetViewModel.token,
                        network: assetViewModel.network,
                        shouldShowNativeTokenIcon: true
                      )
                    }
                  }
                }
              }
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
      .listStyle(.insetGrouped)
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .navigationTitle(Strings.Wallet.searchTitle.capitalized)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: {
            presentationMode.dismiss()
          }) {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
        ToolbarItemGroup(placement: .bottomBar) {
          networkFilterButton
          Spacer()
        }
      }
      .animation(nil, value: query)
      .searchable(
        text: $query,
        placement: .navigationBarDrawer(displayMode: .always)
      )
    }
    .navigationViewStyle(StackNavigationViewStyle())
    .onAppear {
      Task { @MainActor in
        self.allAssets = await userAssetsStore.allAssets()
        self.allNFTMetadata = await userAssetsStore.allNFTMetadata()
      }
    }
  }
  
  private func title(for token: BraveWallet.BlockchainToken) -> String {
    if (token.isErc721 || token.isNft), !token.tokenId.isEmpty {
      return token.nftTokenTitle
    } else {
      return token.name
    }
  }
}

struct SearchAssetView<ImageView: View>: View {
  var image: () -> ImageView
  var title: String
  var symbol: String
  let networkName: String
  
  init(
    title: String,
    symbol: String,
    networkName: String,
    @ViewBuilder image: @escaping () -> ImageView
  ) {
    self.title = title
    self.symbol = symbol
    self.networkName = networkName
    self.image = image
  }

  var body: some View {
    HStack {
      image()
      VStack(alignment: .leading) {
        Text(title)
          .font(.footnote)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        Text(String.localizedStringWithFormat(Strings.Wallet.userAssetSymbolNetworkDesc, symbol, networkName))
          .font(.caption)
          .foregroundColor(Color(.braveLabel))
      }
      Spacer()
    }
    .frame(maxWidth: .infinity)
    .padding(.vertical, 6)
    .accessibilityElement()
    .accessibilityLabel("\(title), \(symbol), \(networkName)")
  }
}
