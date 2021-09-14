/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveCore
import BraveUI

@available(iOS 14.0, *)
struct CryptoSwapView: View {
  @ObservedObject var ethNetworkStore: EthNetworkStore
  @State private var fromQuantity: String = ""
  @State private var toQuantity: String = ""
  @State private var orderType: OrderType = .market
  
  private var fromAssetView: some View {
    VStack {
      HStack(alignment: .top, spacing: 12) {
        TextField("From", text: $fromQuantity)
          .textFieldStyle(BraveTextFieldStyle())
        VStack(alignment: .trailing, spacing: 4) {
          Text("Balance: 30404.22") // NSLocalizedString & formatting
            .foregroundColor(Color(.secondaryBraveLabel))
          HStack {
            Circle()
              .frame(width: 24, height: 24)
            Text("ETH")
              .font(.headline.bold())
            Image(systemName: "chevron.down")
          }
        }
      }
    }
    .foregroundColor(Color(.bravePrimary))
    .padding(12)
    .background(Color(.secondaryBraveGroupedBackground))
    .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
    .frame(maxWidth: .infinity)
  }
  
  private var toAssetView: some View {
    VStack {
      HStack(alignment: .top, spacing: 12) {
        TextField("To", text: $toQuantity)
          .textFieldStyle(BraveTextFieldStyle())
        VStack(alignment: .trailing, spacing: 4) {
          Text("Balance: 0") // NSLocalizedString & formatting
            .foregroundColor(Color(.secondaryBraveLabel))
          HStack {
            Circle()
              .frame(width: 24, height: 24)
            Text("BAT")
              .font(.headline.bold())
            Image(systemName: "chevron.down")
          }
        }
      }
    }
    .foregroundColor(Color(.bravePrimary))
    .padding(12)
    .background(Color(.secondaryBraveGroupedBackground))
    .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
    .frame(maxWidth: .infinity)
  }
  
  enum OrderType {
    case market
    case limit
  }
  
  var body: some View {
    NavigationView {
      ScrollView(.vertical) {
        VStack {
          HStack {
            AccountView(address: "0x", name: "Account 1")
            NetworkPicker(
              networks: ethNetworkStore.ethereumChains,
              selectedNetwork: ethNetworkStore.selectedChainBinding
            )
          }
          .frame(maxWidth: .infinity)
          fromAssetView
          Image(systemName: "arrow.down")
          toAssetView
          Picker("Order type", selection: $orderType) {
            Text("Market").tag(OrderType.market)
            Text("Limit").tag(OrderType.limit)
          }
          .pickerStyle(SegmentedPickerStyle())
          .scaledToFill()
        }
        .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
      }
      .navigationTitle("Swap") // NSLocalizedString
      .navigationBarTitleDisplayMode(.inline)
      .padding(12)
      .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: { }) {
            Text("Cancel") // NSLocalizedString
              .foregroundColor(.accentColor)
          }
        }
      }
    }
  }
}

#if DEBUG
@available(iOS 14.0, *)
struct CryptoSwapView_Previews: PreviewProvider {
  static var previews: some View {
    CryptoSwapView(ethNetworkStore: .previewStore)
  }
}
#endif
