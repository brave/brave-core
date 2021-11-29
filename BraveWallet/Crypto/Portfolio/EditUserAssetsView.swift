// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import struct Shared.Strings
import BraveShared
import BraveCore

private struct EditTokenView: View {
  @ObservedObject var assetStore: AssetStore
  
  var body: some View {
    Button(action: {
      assetStore.isVisible.toggle()
    }) {
      HStack(spacing: 8) {
        AssetIconView(token: assetStore.token)
        VStack(alignment: .leading) {
          Text(assetStore.token.name)
            .fontWeight(.semibold)
            .foregroundColor(Color(.bravePrimary))
          Text(assetStore.token.symbol.uppercased())
            .foregroundColor(Color(.secondaryBraveLabel))
        }
        .font(.footnote)
        Spacer()
        Image(systemName: "checkmark")
          .opacity(assetStore.isVisible ? 1 : 0)
      }
      .padding(.vertical, 8)
    }
  }
}

struct EditUserAssetsView: View {
  @ObservedObject var userAssetsStore: UserAssetsStore
  var assetsUpdated: () -> Void
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  @State private var query = ""
  @State private var isAddingCustomAsset = false
  @State private var isPresentingAssetRemovalError = false
  
  private var tokenStores: [AssetStore] {
    let query = query.lowercased()
    var stores = userAssetsStore.assetStores
    if !query.isEmpty {
      stores = stores.filter {
        $0.token.symbol.lowercased().contains(query) ||
        $0.token.name.lowercased().contains(query)
      }
    }
    return stores
      .sorted(by: { $0.token.symbol.caseInsensitiveCompare($1.token.symbol) == .orderedAscending })
      .sorted(by: { $0.isVisible && !$1.isVisible })
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
            Button(action: {
              isAddingCustomAsset = true
            }) {
              Text(Strings.Wallet.addCustomAsset)
                .font(.footnote.weight(.bold))
                .textCase(.none)
                .foregroundColor(Color(.braveBlurpleTint))
            }
          }
            .osAvailabilityModifiers { content in
              if #available(iOS 15.0, *) {
                content // Padding already applied
              } else {
                content
                  .padding(.top)
              }
            }
        ) {
          let tokens = tokenStores
          if tokens.isEmpty {
            Text(Strings.Wallet.assetSearchEmpty)
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
              .multilineTextAlignment(.center)
              .frame(maxWidth: .infinity)
          } else {
            ForEach(tokens, id: \.token.id) { store in
              if store.isCustomToken {
                EditTokenView(assetStore: store)
                  .osAvailabilityModifiers { content in
                    if #available(iOS 15.0, *) {
                      content
                        .swipeActions(edge: .trailing) {
                          Button(role: .destructive) {
                            removeCustomToken(store.token)
                          } label: {
                            Label(Strings.Wallet.deleteCustomToken, systemImage: "trash")
                          }
                        }
                    } else {
                      content
                        .contextMenu {
                          Button {
                            removeCustomToken(store.token)
                          } label: {
                            Label(Strings.Wallet.deleteCustomToken, systemImage: "trash")
                          }
                        }
                    }
                  }
              } else {
                EditTokenView(assetStore: store)
              }
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .animation(.default, value: tokenStores)
      .navigationTitle(Strings.Wallet.editVisibleAssetsButtonTitle)
      .navigationBarTitleDisplayMode(.inline)
      .filterable(text: $query)
      .toolbar {
        ToolbarItemGroup(placement: .confirmationAction) {
          Button(action: {
            assetsUpdated()
            presentationMode.dismiss()
          }) {
            Text(Strings.done)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
      .sheet(isPresented: $isAddingCustomAsset) {
        AddCustomAssetView(userAssetStore: userAssetsStore)
      }
      .alert(isPresented: $isPresentingAssetRemovalError) {
        Alert(
          title: Text(Strings.Wallet.removeCustomTokenErrorTitle),
          message: Text(Strings.Wallet.removeCustomTokenErrorMessage),
          dismissButton: .default(Text(Strings.OKString))
        )
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
  }
  
  private func removeCustomToken(_ token: BraveWallet.ERCToken) {
    userAssetsStore.removeUserAsset(token: token) { [self] success in
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
