// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import struct Shared.Strings
import BraveShared
import BraveCore

struct AddSuggestedNetworkView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  var chain: BraveWallet.NetworkInfo
  
  @ScaledMetric private var blockieSize = 24
  @ScaledMetric private var faviconSize = 48
  
  @Environment(\.openWalletURLAction) private var openWalletURL
  
  private var headerView: some View {
    VStack {
      HStack(spacing: 8) {
        Spacer()
        Text(keyringStore.selectedAccount.address.truncatedAddress)
          .fontWeight(.semibold)
        Blockie(address: keyringStore.selectedAccount.address)
          .frame(width: blockieSize, height: blockieSize)
      }
      VStack(spacing: 8) {
        Image(systemName: "globe")
          .frame(width: faviconSize, height: faviconSize)
          .background(Color(.braveDisabled))
          .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
        Text(verbatim: "https://app.uniswap.org")
          .font(.subheadline)
          .foregroundColor(Color(.braveLabel))
          .multilineTextAlignment(.center)
        Text("Allow this site to add a network?")
          .font(.headline)
          .foregroundColor(Color(.bravePrimary))
          .multilineTextAlignment(.center)
        Text("This will allow this network to be used within Brave Wallet.")
          .font(.subheadline)
          .foregroundColor(Color(.braveLabel))
          .multilineTextAlignment(.center)
        Button {
          openWalletURL?(BraveUX.braveWalletNetworkLearnMoreURL)
        } label: {
          Text("Learn More")
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
      .frame(maxWidth: .infinity)
      .padding(.vertical)
    }
    .resetListHeaderStyle()
    .padding(.vertical)
  }
  
  var body: some View {
    NavigationView {
      List {
        Section {
          VStack(alignment: .leading) {
            Text("Network Name")
              .fontWeight(.semibold)
            Text("BSC (Binance Smart Chain)")
          }
          .padding(.vertical, 6)
          if let networkURL = chain.rpcUrls.first {
            VStack(alignment: .leading) {
              Text("Network URL")
                .fontWeight(.semibold)
              Text(networkURL)
            }
            .padding(.vertical, 6)
          }
          if let blockExplorerURL = chain.blockExplorerUrls.first?.asURL {
            Button {
              // Opens the block explorer URL for the network
              openWalletURL?(blockExplorerURL)
            } label: {
              Text("View Details")
                .foregroundColor(Color(.braveBlurpleTint))
            }
          }
        } header: {
          headerView
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        .font(.footnote)
        
        Section {
          Button {
            
          } label: {
            HStack {
              Image("brave.checkmark.circle.fill")
              Text("Approve")
            }
          }
          .buttonStyle(BraveFilledButtonStyle(size: .large))
          .frame(maxWidth: .infinity)
        }
        .listRowBackground(Color(.braveGroupedBackground))
      }
      .navigationTitle("Add Network")
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button {
            
          } label: {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
    .navigationViewStyle(.stack)
  }
}

#if DEBUG
struct AddSuggestedNetworkView_Previews: PreviewProvider {
  static var previews: some View {
    AddSuggestedNetworkView(
      keyringStore: .previewStoreWithWalletCreated,
      networkStore: .previewStore,
      chain: .mockRopsten
    )
  }
}
#endif
