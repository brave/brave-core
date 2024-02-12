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
  @ObservedObject var networkStore: NetworkStore
  var cryptoStore: CryptoStore
  var userAssetsStore: UserAssetsStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var allAssets: [AssetViewModel] = []
  @State private var allNFTMetadata: [String: NFTMetadata] = [:]
  @State private var query = ""
  @State private var networkFilters: [Selectable<BraveWallet.NetworkInfo>] = []
  @State private var isPresentingNetworkFilter = false
  @State private var selectedToken: BraveWallet.BlockchainToken?
  @State private var isLoadingMetadata: Bool = false
  
  public init(
    keyringStore: KeyringStore,
    cryptoStore: CryptoStore,
    userAssetsStore: UserAssetsStore
  ) {
    self.keyringStore = keyringStore
    self.networkStore = cryptoStore.networkStore
    self.cryptoStore = cryptoStore
    self.userAssetsStore = userAssetsStore
  }

  private var filteredTokens: [AssetViewModel] {
    let filterByNetwork = !networkFilters.allSatisfy(\.isSelected)
    let filterByQuery = !query.isEmpty
    if !filterByNetwork && !filterByQuery {
      return allAssets
    }
    let selectedNetworks = networkFilters.filter(\.isSelected)
    let normalizedQuery = query.lowercased()
    return allAssets.filter { asset in
      if filterByNetwork,
         !selectedNetworks.contains(where: { asset.network.chainId == $0.model.chainId }) {
        return false
      }
      if filterByQuery {
        return asset.token.symbol.lowercased().contains(normalizedQuery) || asset.token.name.lowercased().contains(normalizedQuery)
      }
      return true
    }
  }
  
  private var networkFilterButton: some View {
    Button(action: {
      self.isPresentingNetworkFilter = true
    }) {
      Image(braveSystemName: "leo.tune")
        .font(.footnote.weight(.medium))
        .foregroundColor(Color(.braveBlurpleTint))
        .clipShape(Rectangle())
    }
    .sheet(isPresented: $isPresentingNetworkFilter) {
      NavigationView {
        NetworkFilterView(
          networks: networkFilters,
          networkStore: cryptoStore.networkStore,
          saveAction: { networkFilters in
            self.networkFilters = networkFilters
          }
        )
      }
      .navigationViewStyle(.stack)
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
                Button(action: { selectedToken = assetViewModel.token }) {
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
                        shouldShowNetworkIcon: true,
                        isLoadingMetadata: isLoadingMetadata
                      )
                    } else {
                      AssetIconView(
                        token: assetViewModel.token,
                        network: assetViewModel.network,
                        shouldShowNetworkIcon: true
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
      .background(
        NavigationLink(
          isActive: Binding(
            get: { selectedToken != nil },
            set: { if !$0 { selectedToken = nil } }
          ),
          destination: {
            if let selectedToken {
              if selectedToken.isErc721 || selectedToken.isNft {
                NFTDetailView(
                  keyringStore: keyringStore,
                  nftDetailStore: cryptoStore.nftDetailStore(for: selectedToken, nftMetadata: allNFTMetadata[selectedToken.id], owner: nil),
                  buySendSwapDestination: .constant(nil)
                ) { metadata in
                  allNFTMetadata[selectedToken.id] = metadata
                }
                .onDisappear {
                  cryptoStore.closeNFTDetailStore(for: selectedToken)
                }
              } else {
                AssetDetailView(
                  assetDetailStore: cryptoStore.assetDetailStore(for: .blockchainToken(selectedToken)),
                  keyringStore: keyringStore,
                  networkStore: cryptoStore.networkStore
                )
                .onDisappear {
                  cryptoStore.closeAssetDetailStore(for: .blockchainToken(selectedToken))
                }
              }
            }
          },
          label: {
            EmptyView()
          })
      )
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
        self.isLoadingMetadata = true
        self.allNFTMetadata = await userAssetsStore.allNFTMetadata()
        self.isLoadingMetadata = false
        self.networkFilters = networkStore.allChains.map {
          .init(isSelected: true, model: $0)
        }
      }
    }
    .onChange(of: networkStore.allChains) { allChains in
      self.networkFilters = allChains.map { network in
        let existingSelectionValue = self.networkFilters.first(where: { $0.model.chainId == network.chainId})?.isSelected
        return .init(isSelected: existingSelectionValue ?? true, model: network)
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
      Image(systemName: "chevron.right")
        .font(.body.weight(.semibold))
        .foregroundColor(Color(.separator))
    }
    .frame(maxWidth: .infinity)
    .padding(.vertical, 6)
    .accessibilityElement()
    .accessibilityLabel("\(title), \(symbol), \(networkName)")
  }
}
