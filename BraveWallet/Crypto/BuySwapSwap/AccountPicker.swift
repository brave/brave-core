// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import struct Shared.Strings

struct AccountPicker: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @Binding var account: BraveWallet.AccountInfo
  
  @State private var isPresentingPicker: Bool = false
  @Environment(\.sizeCategory) private var sizeCategory
  @ScaledMetric private var avatarSize = 24.0
  
  var body: some View {
    Group {
      if sizeCategory.isAccessibilityCategory {
        VStack(alignment: .leading) {
          networkPickerView
          accountPickerView
        }
        .frame(maxWidth: .infinity, alignment: .leading)
      } else {
        HStack {
          accountPickerView
          Spacer()
          networkPickerView
        }
      }
    }
    .sheet(isPresented: $isPresentingPicker) {
      pickerList
    }
  }
  
  private var accountView: some View {
    HStack {
      Blockie(address: account.address)
        .frame(width: avatarSize, height: avatarSize)
      VStack(alignment: .leading, spacing: 2) {
        Text(account.name)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
          .multilineTextAlignment(.leading)
        Text(account.address.truncatedAddress)
          .foregroundColor(Color(.braveLabel))
          .multilineTextAlignment(.leading)
      }
      .font(.caption)
      Image(systemName: "chevron.down.circle")
        .font(.footnote.weight(.medium))
        .foregroundColor(Color(.primaryButtonTint))
    }
    .padding(.vertical, 6)
  }
  
  private func copyAddress() {
    UIPasteboard.general.string = account.address
  }
  
  private var menuContents: some View {
    Button(action: copyAddress) {
      Label(Strings.Wallet.copyAddressButtonTitle, image: "brave.clipboard")
    }
  }
  
  @ViewBuilder private var accountPickerView: some View {
    if #available(iOS 15.0, *) {
      Menu {
        menuContents
      } label: {
        accountView
      } primaryAction: {
        isPresentingPicker = true
      }
    } else {
      Button(action: {
        isPresentingPicker = true
      }) {
        accountView
      }
      .background(Color(.braveBackground)) // For the contextMenu
      .contextMenu {
        menuContents
      }
    }
  }
  
  private var networkPickerView: some View {
    NetworkPicker(
      networks: networkStore.ethereumChains,
      selectedNetwork: networkStore.selectedChainBinding
    )
  }
  
  private var pickerList: some View {
    NavigationView {
      List {
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.accountsPageTitle))
        ) {
          ForEach(keyringStore.keyring.accountInfos) { account in
            Button(action: {
              self.account = account
              isPresentingPicker = false
            }) {
              AccountView(address: account.address, name: account.name)
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .navigationTitle(Strings.Wallet.selectAccountTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: { isPresentingPicker = false }) {
            Text(Strings.CancelString)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
  }
}

#if DEBUG
struct AccountPicker_Previews: PreviewProvider {
  static var previews: some View {
    AccountPicker(
      keyringStore: .previewStoreWithWalletCreated,
      networkStore: .previewStore,
      account: .constant(.previewAccount)
    )
    .padding()
    .previewLayout(.sizeThatFits)
    .previewSizeCategories()
  }
}
#endif
