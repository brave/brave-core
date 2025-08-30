// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Strings
import SwiftUI

struct AssetSearchView: View {
  var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  var cryptoStore: CryptoStore
  var userAssetsStore: UserAssetsStore

  @Environment(\.presentationMode) @Binding private var presentationMode

  @State private var allAssets: [AssetViewModel] = []
  @State private var allNFTMetadata: [String: BraveWallet.NftMetadata] = [:]
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

  private var filteredTokensByNetworks: [AssetViewModel] {
    let filterByNetwork = !networkFilters.allSatisfy(\.isSelected)
    if !filterByNetwork {
      return allAssets
    }
    let selectedNetworks = networkFilters.filter(\.isSelected)
    return allAssets.filter { asset in
      if filterByNetwork,
        !selectedNetworks.contains(where: { asset.network.chainId == $0.model.chainId })
      {
        return false
      }
      return true
    }
  }

  private var networkFilterButton: some View {
    Button {
      self.isPresentingNetworkFilter = true
    } label: {
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
      TokenList(
        tokens: filteredTokensByNetworks
      ) { query, viewModel in
        viewModel.token.symbol.localizedCaseInsensitiveContains(query)
          || viewModel.token.name.localizedCaseInsensitiveContains(query)
      } header: {
        TokenListHeaderView(title: Strings.Wallet.assetsTitle)
      } emptyStateView: {
        Text(Strings.Wallet.assetSearchEmpty)
          .font(.footnote)
          .foregroundColor(Color(.secondaryBraveLabel))
          .multilineTextAlignment(.center)
          .frame(maxWidth: .infinity)
      } content: { viewModel in
        Button {
          selectedToken = viewModel.token
        } label: {
          SearchAssetView(
            title: title(for: viewModel.token),
            symbol: viewModel.token.symbol,
            networkName: viewModel.network.chainName
          ) {
            if viewModel.token.isErc721 || viewModel.token.isNft {
              NFTIconView(
                token: viewModel.token,
                network: viewModel.network,
                url: allNFTMetadata[viewModel.token.id]?.imageURL,
                shouldShowNetworkIcon: true,
                isLoadingMetadata: isLoadingMetadata
              )
            } else {
              AssetIconView(
                token: viewModel.token,
                network: viewModel.network,
                shouldShowNetworkIcon: true
              )
            }
          }
        }
      }
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
                  nftDetailStore: cryptoStore.nftDetailStore(
                    for: selectedToken,
                    nftMetadata: allNFTMetadata[selectedToken.id],
                    owner: nil
                  ),
                  walletActionDestination: .constant(nil)
                ) { metadata in
                  allNFTMetadata[selectedToken.id] = metadata
                }
                .onDisappear {
                  cryptoStore.closeNFTDetailStore(for: selectedToken)
                }
              } else {
                AssetDetailView(
                  assetDetailStore: cryptoStore.assetDetailStore(
                    for: .blockchainToken(selectedToken)
                  ),
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
          }
        )
      )
      .navigationTitle(Strings.Wallet.searchTitle.capitalized)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button {
            presentationMode.dismiss()
          } label: {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
        ToolbarItemGroup(placement: .bottomBar) {
          networkFilterButton
          Spacer()
        }
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
    .onAppear {
      Task { @MainActor in
        self.allAssets = await userAssetsStore.allAssets()
        self.isLoadingMetadata = true
        self.allNFTMetadata = await userAssetsStore.allNFTMetadata()
        self.isLoadingMetadata = false
        self.networkFilters = networkStore.visibleChains.map {
          .init(isSelected: true, model: $0)
        }
      }
    }
    .onChange(of: networkStore.allChains) { allChains in
      self.networkFilters = allChains.map { network in
        let existingSelectionValue = self.networkFilters.first(where: {
          $0.model.chainId == network.chainId
        })?.isSelected
        return .init(isSelected: existingSelectionValue ?? true, model: network)
      }
    }
  }

  private func title(for token: BraveWallet.BlockchainToken) -> String {
    if token.isErc721 || token.isNft, !token.tokenId.isEmpty {
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
        Text(
          String.localizedStringWithFormat(
            Strings.Wallet.userAssetSymbolNetworkDesc,
            symbol,
            networkName
          )
        )
        .font(.caption)
        .foregroundColor(Color(.braveLabel))
      }
      Spacer()
      Image(systemName: "chevron.right")
        .font(.body.weight(.semibold))
        .foregroundColor(Color(.separator))
    }
    .padding(.vertical, 6)
    .accessibilityElement()
    .accessibilityLabel("\(title), \(symbol), \(networkName)")
  }
}
