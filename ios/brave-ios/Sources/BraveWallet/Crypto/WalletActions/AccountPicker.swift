// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Strings
import SwiftUI

struct AccountPicker: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore

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
      NavigationView {
        AccountSelectionView(
          keyringStore: keyringStore,
          networkStore: networkStore,
          onDismiss: { isPresentingPicker = false }
        )
      }
      .navigationViewStyle(.stack)
    }
  }

  private var accountView: some View {
    HStack {
      Blockie(address: keyringStore.selectedAccount.blockieSeed)
        .frame(width: avatarSize, height: avatarSize)
      VStack(alignment: .leading, spacing: 2) {
        Text(keyringStore.selectedAccount.name)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
          .multilineTextAlignment(.leading)
        if !keyringStore.selectedAccount.address.isEmpty {
          Text(keyringStore.selectedAccount.address.truncatedAddress)
            .foregroundColor(Color(.braveLabel))
            .multilineTextAlignment(.leading)
        }
      }
      .font(.caption)
      Image(systemName: "chevron.down.circle")
        .font(.footnote.weight(.medium))
        .foregroundColor(Color(.primaryButtonTint))
    }
    .padding(.vertical, 6)
  }

  private func copyAddress() {
    UIPasteboard.general.string = keyringStore.selectedAccount.address
  }

  @ViewBuilder private var accountPickerView: some View {
    Menu {
      Text(keyringStore.selectedAccount.address.zwspOutput)
      Button(action: copyAddress) {
        Label(Strings.Wallet.copyAddressButtonTitle, braveSystemImage: "leo.copy.plain-text")
      }
    } label: {
      accountView
    } primaryAction: {
      isPresentingPicker = true
    }
    .accessibilityLabel(Strings.Wallet.selectedAccountAccessibilityLabel)
    .accessibilityValue(
      "\(keyringStore.selectedAccount.name), \(keyringStore.selectedAccount.address.truncatedAddress)"
    )
  }

  private var networkPickerView: some View {
    NetworkPicker(
      keyringStore: keyringStore,
      networkStore: networkStore
    )
    .buttonStyle(.plain)
  }
}

#if DEBUG
struct AccountPicker_Previews: PreviewProvider {
  static var previews: some View {
    AccountPicker(
      keyringStore: .previewStoreWithWalletCreated,
      networkStore: .previewStore
    )
    .padding()
    .previewLayout(.sizeThatFits)
    .previewSizeCategories()
  }
}
#endif
