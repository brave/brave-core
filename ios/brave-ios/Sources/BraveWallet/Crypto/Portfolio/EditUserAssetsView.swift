// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Foundation
import Strings
import SwiftUI

private struct EditTokenView: View {
  @ObservedObject var assetStore: AssetStore

  @Binding var tokenNeedsTokenId: BraveWallet.BlockchainToken?

  @State var nftMetadata: BraveWallet.NftMetadata?

  private var tokenName: String {
    if assetStore.token.isErc721 || assetStore.token.isNft, !assetStore.token.tokenId.isEmpty {
      return assetStore.token.nftTokenTitle
    } else {
      return assetStore.token.name
    }
  }

  var body: some View {
    Button {
      if assetStore.token.isErc721, assetStore.token.tokenId.isEmpty {
        tokenNeedsTokenId = assetStore.token
      } else {
        assetStore.isVisible.toggle()
      }
    } label: {
      HStack(spacing: 8) {
        if assetStore.token.isErc721 || assetStore.token.isNft {
          NFTIconView(
            token: assetStore.token,
            network: assetStore.network,
            url: nftMetadata?.imageURL,
            shouldShowNetworkIcon: true
          )
        } else {
          AssetIconView(
            token: assetStore.token,
            network: assetStore.network,
            shouldShowNetworkIcon: true
          )
        }
        VStack(alignment: .leading) {
          Text(tokenName)
            .fontWeight(.semibold)
            .foregroundColor(Color(.bravePrimary))
          Text(
            String.localizedStringWithFormat(
              Strings.Wallet.userAssetSymbolNetworkDesc,
              assetStore.token.symbol,
              assetStore.network.chainName
            )
          )
          .foregroundColor(Color(.secondaryBraveLabel))
        }
        .font(.footnote)
        Spacer()
        Image(systemName: "checkmark")
          .opacity(assetStore.isVisible ? 1 : 0)
      }
      .padding(.vertical, 8)
    }
    .onAppear {
      Task { @MainActor in
        self.nftMetadata = await assetStore.fetchERC721Metadata()
      }
    }
    .accessibilityAddTraits(assetStore.isVisible ? [.isSelected] : [])
  }
}

struct EditUserAssetsView: View {
  var networkStore: NetworkStore
  var keyringStore: KeyringStore
  @ObservedObject var userAssetsStore: UserAssetsStore

  @Environment(\.presentationMode) @Binding private var presentationMode
  @State private var query = ""
  @State private var isAddingCustomAsset = false
  @State private var isPresentingAssetRemovalError = false
  @State private var tokenNeedsTokenId: BraveWallet.BlockchainToken?
  @State private var isPresentingNetworkFilter = false

  private var tokenStores: [AssetStore] {
    let normalizedQuery = query.lowercased()
    var stores = userAssetsStore.assetStores
    if !normalizedQuery.isEmpty {
      stores = stores.filter {
        $0.token.symbol.localizedCaseInsensitiveContains(normalizedQuery)
          || $0.token.name.localizedCaseInsensitiveContains(normalizedQuery)
      }
    }
    return stores.sorted(by: { $0.isVisible && !$1.isVisible })
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
  }

  private var addCustomAssetButton: some View {
    Button {
      isAddingCustomAsset = true
    } label: {
      Image(systemName: "plus")
    }
  }

  var body: some View {
    NavigationView {
      List {
        Section(
          header: HStack {
            WalletListHeaderView(
              title: Text(Strings.Wallet.assetsTitle)
            )
            Spacer()
          }
        ) {
          Group {
            let tokens = tokenStores
            if tokens.isEmpty {
              Text(Strings.Wallet.assetSearchEmpty)
                .font(.footnote)
                .foregroundColor(Color(.secondaryBraveLabel))
                .multilineTextAlignment(.center)
                .frame(maxWidth: .infinity)
            } else {
              ForEach(tokens, id: \.token.id) { store in
                if store.isRemovable {
                  EditTokenView(assetStore: store, tokenNeedsTokenId: $tokenNeedsTokenId)
                    .swipeActions(edge: .trailing) {
                      Button(role: .destructive) {
                        removeToken(store.token)
                      } label: {
                        Label(Strings.Wallet.delete, systemImage: "trash")
                      }
                    }
                } else {
                  EditTokenView(assetStore: store, tokenNeedsTokenId: $tokenNeedsTokenId)
                }
              }
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .animation(.default, value: tokenStores)
      .navigationTitle(Strings.Wallet.editVisibleAssetsButtonTitle)
      .navigationBarTitleDisplayMode(.inline)
      .navigationViewStyle(StackNavigationViewStyle())
      .searchable(
        text: $query,
        placement: .navigationBarDrawer(displayMode: .always)
      )
      .onAppear {
        userAssetsStore.update()
      }
      .toolbar {
        ToolbarItemGroup(placement: .bottomBar) {
          networkFilterButton
          Spacer()
          addCustomAssetButton
        }
        ToolbarItemGroup(placement: .confirmationAction) {
          Button {
            presentationMode.dismiss()
          } label: {
            Text(Strings.done)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }  // List
    }  // NavigationView
    .background(
      Color.clear.sheet(
        isPresented: Binding(
          get: { tokenNeedsTokenId != nil },
          set: { if !$0 { tokenNeedsTokenId = nil } }
        )
      ) {
        AddCustomAssetView(
          networkStore: networkStore,
          networkSelectionStore: networkStore.openNetworkSelectionStore(mode: .formSelection),
          keyringStore: keyringStore,
          userAssetStore: userAssetsStore,
          tokenNeedsTokenId: tokenNeedsTokenId,
          supportedTokenTypes: [.nft]
        )
        .onDisappear {
          networkStore.closeNetworkSelectionStore()
        }
      }
    )
    .background(
      Color.clear.alert(isPresented: $isPresentingAssetRemovalError) {
        Alert(
          title: Text(Strings.Wallet.removeCustomTokenErrorTitle),
          message: Text(Strings.Wallet.removeCustomTokenErrorMessage),
          dismissButton: .default(Text(Strings.OKString))
        )
      }
    )
    .background(
      Color.clear.sheet(isPresented: $isPresentingNetworkFilter) {
        NavigationView {
          NetworkFilterView(
            networks: userAssetsStore.networkFilters,
            networkStore: networkStore,
            saveAction: { selectedNetworks in
              userAssetsStore.networkFilters = selectedNetworks
            }
          )
        }
        .navigationViewStyle(.stack)
        .onDisappear {
          networkStore.closeNetworkSelectionStore()
        }
      }
    )
    .background(
      Color.clear.sheet(isPresented: $isAddingCustomAsset) {
        AddCustomAssetView(
          networkStore: networkStore,
          networkSelectionStore: networkStore.openNetworkSelectionStore(mode: .formSelection),
          keyringStore: keyringStore,
          userAssetStore: userAssetsStore
        )
        .onDisappear {
          networkStore.closeNetworkSelectionStore()
        }
      }
    )
  }

  private func removeToken(_ token: BraveWallet.BlockchainToken) {
    Task { @MainActor in
      let success = await userAssetsStore.removeUserAsset(token: token)
      isPresentingAssetRemovalError = !success
    }
  }
}

#if DEBUG
struct EditUserAssetsView_Previews: PreviewProvider {
  static var previews: some View {
    EmptyView()
  }
}
#endif
