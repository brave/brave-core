// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import OrderedCollections
import SwiftUI

struct BuyProviderSelectionView: View {
  @ObservedObject var buyTokenStore: BuyTokenStore
  @ObservedObject var keyringStore: KeyringStore

  @Environment(\.openURL) private var openWalletURL

  @ScaledMetric private var iconSize = 40.0
  private let maxIconSize: CGFloat = 80.0

  var body: some View {
    List {
      Section(
        header: WalletListHeaderView(
          title: Text(Strings.Wallet.providerSelectionSectionHeader)
        )
      ) {
        ForEach(buyTokenStore.supportedProviders) { provider in
          VStack(alignment: .leading, spacing: 12) {
            HStack(spacing: 12) {
              Image(provider.iconName, bundle: .module)
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(width: min(iconSize, maxIconSize), height: min(iconSize, maxIconSize))
              VStack(alignment: .leading, spacing: 12) {
                Text(provider.name)
                  .foregroundColor(Color(.bravePrimary))
                  .font(.headline)
                  .multilineTextAlignment(.leading)
                Text(provider.localizedDescription)
                  .foregroundColor(Color(.bravePrimary))
                  .font(.footnote)
                  .multilineTextAlignment(.leading)
              }
            }
            HStack(spacing: 12) {
              Spacer()
                .frame(width: min(iconSize, maxIconSize), height: min(iconSize, maxIconSize))
              Button {
                Task { @MainActor in
                  guard
                    let url = await buyTokenStore.fetchBuyUrl(
                      provider: provider,
                      account: keyringStore.selectedAccount
                    )
                  else { return }
                  openWalletURL(url)
                }
              } label: {
                Text(
                  String.localizedStringWithFormat(
                    Strings.Wallet.providerSelectionButtonTitle,
                    provider.shortName
                  )
                )
                .foregroundColor(Color(.bravePrimary))
              }
              .buttonStyle(BraveOutlineButtonStyle(size: .normal))
              Spacer()
            }
          }
          .padding(.vertical, 10)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.Wallet.providerSelectionScreenTitle)
  }
}

#if DEBUG
struct BuyProviderSelectionView_Previews: PreviewProvider {
  static var previews: some View {
    BuyProviderSelectionView(
      buyTokenStore: .previewStore,
      keyringStore: .previewStore
    )
  }
}
#endif
