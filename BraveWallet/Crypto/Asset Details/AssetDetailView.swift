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
  @ObservedObject var networkStore: EthNetworkStore
  
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
        networkStore: .previewStore
      )
        .navigationBarTitleDisplayMode(.inline)
    }
    .previewColorSchemes()
  }
}
#endif

//struct AccountView: View {
//  var address: String
//  var name: String
//
//  var body: some View {
//    HStack {
//      Blockie(address: address)
//        .frame(width: 40, height: 40)
//      VStack(alignment: .leading, spacing: 2) {
//        Text(name)
//          .fontWeight(.semibold)
//        Text("0xFCdF***DDee")
//      }
//      Spacer()
//      VStack(alignment: .trailing, spacing: 2) {
//        Text("$616.47")
//        Text("0.31178 ETH")
//      }
//    }
//    .font(.caption)
//    .padding(12)
//    .frame(maxWidth: .infinity, alignment: .leading)
//  }
//}
