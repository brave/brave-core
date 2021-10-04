/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SwiftUI
import BraveCore
import BraveUI

struct AssetDetailView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  var token: BraveWallet.ERCToken

  var body: some View {
    List {
      Section(
        header: AssetDetailHeaderView(
          keyringStore: keyringStore,
          networkStore: networkStore,
          currency: Currency(image: .init(), name: "Basic Attention Token", symbol: "BAT", cost: 0.999444)
        )
        .resetListHeaderStyle()
        .padding(.horizontal, -16) // inset grouped layout margins workaround
      ) {
      }
      Section(
        header: WalletListHeaderView(title: Text("Accounts")) // NSLocalizedString
          .osAvailabilityModifiers { content in
            if #available(iOS 15.0, *) {
              content // padding already applied
            } else {
              content
            }
          },
        footer: Button(action: {}) {
          Text("Add Account") // NSLocalizedString
        }
        .listRowInsets(.zero)
        .buttonStyle(BraveOutlineButtonStyle(size: .small))
        .padding(.vertical, 8)
      ) {
        Text("No accounts") // NSLocalizedString
      }
      Section(
        header: WalletListHeaderView(title: Text("Transactions")) // NSLocalizedString
      ) {
        Text("No transactions")
      }
      Section(
        header: WalletListHeaderView(title: Text("Info")) // NSLocalizedString
      ) {
        Text("No info") // NSLocalizedString
      }
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle("Basic Attention Token") // TODO: Replace by actual currency
    .navigationBarTitleDisplayMode(.inline)
  }
}

#if DEBUG
struct CurrencyDetailView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      AssetDetailView(
        keyringStore: .previewStore,
        networkStore: .previewStore,
        token: .init(
          contractAddress: "",
          name: "Ethereum",
          logo: "",
          isErc20: false,
          isErc721: false,
          symbol: "ETH",
          decimals: 18,
          visible: true
        )
      )
        .navigationBarTitleDisplayMode(.inline)
    }
    .previewColorSchemes()
  }
}
#endif
