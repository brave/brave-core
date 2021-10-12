/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveCore
import BraveUI
import struct Shared.Strings

struct ShortcutAmountGrid: View {
  enum Amount: Double, CaseIterable {
    case quarter = 0.25
    case half = 0.5
    case threeQuarters = 0.75
    case all = 1.0
    
    var label: String {
      let nf = NumberFormatter()
      nf.numberStyle = .percent
      return nf.string(from: NSNumber(value: rawValue)) ?? ""
    }
  }
  
  var action: (Amount) -> Void
  
  @Environment(\.sizeCategory) private var sizeCategory
  
  private var backgroundShape: some InsettableShape {
    RoundedRectangle(cornerRadius: 10, style: .continuous)
  }
  
  var body: some View {
    HStack(spacing: 8) {
      ForEach(Amount.allCases, id: \.rawValue) { amount in
        Button(action: { action(amount) }) {
          Text(amount.label)
            .lineLimit(1)
            .minimumScaleFactor(0.75)
            .padding(.vertical, 12)
            .frame(maxWidth: .infinity)
            .foregroundColor(Color(.secondaryBraveLabel))
            .background(
              backgroundShape
                .fill(Color(.secondaryBraveGroupedBackground))
                .overlay(
                  // When using an accessibility font size, inset grouped tables automatically change to
                  // grouped tables with separators. So we will match this change and add a border around
                  // the buttons to make them appear more uniform with the table
                  Group {
                    if sizeCategory.isAccessibilityCategory {
                      backgroundShape
                        .strokeBorder(Color(.separator))
                    }
                  }
                )
            )
            .padding(.top, 8)
        }
      }
    }
  }
}

struct SwapCryptoView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var ethNetworkStore: NetworkStore
  
  @State private var fromQuantity: String = ""
  @State private var toQuantity: String = ""
  @State private var orderType: OrderType = .market
  
  enum OrderType {
    case market
    case limit
  }
  
  var body: some View {
    NavigationView {
      Form {
        Section(
          header: AccountPicker(
            keyringStore: keyringStore,
            networkStore: ethNetworkStore,
            account: .constant(keyringStore.keyring.accountInfos.first!)
          )
            .listRowBackground(Color.clear)
            .resetListHeaderStyle()
            .padding(.top)
            .padding(.bottom, -16) // Get it a bit closer
        ) {
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.swapCryptoFromTitle))
        ) {
          NavigationLink(destination: EmptyView()) {
            HStack {
              Circle()
                .frame(width: 26, height: 26)
              Text(verbatim: "BAT")
                .font(.title3.weight(.semibold))
                .foregroundColor(Color(.braveLabel))
              Spacer()
              Text(verbatim: "1.2832")
                .font(.footnote)
                .foregroundColor(Color(.secondaryBraveLabel))
            }
            .padding(.vertical, 8)
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.swapCryptoAmountTitle)),
          footer: ShortcutAmountGrid(action: { amount in
            
          })
          .listRowInsets(.zero)
          .padding(.bottom, 8)
        ) {
          TextField(Strings.Wallet.swapCryptoAmountPlaceholder, text: .constant(""))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.swapCryptoToTitle))
        ) {
          NavigationLink(destination: EmptyView()) {
            HStack {
              Circle()
                .frame(width: 26, height: 26)
              Text(verbatim: "ETH")
                .font(.title3.weight(.semibold))
                .foregroundColor(Color(.braveLabel))
              Spacer()
              Text(verbatim: "0.0000")
                .font(.footnote)
                .foregroundColor(Color(.secondaryBraveLabel))
            }
            .padding(.vertical, 8)
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.swapCryptoAmountReceivingTitle))
        ) {
          TextField(Strings.Wallet.swapCryptoAmountPlaceholder, text: .constant(""))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: Picker(Strings.Wallet.swapOrderTypeLabel, selection: $orderType) {
            Text(Strings.Wallet.swapMarketOrderType).tag(OrderType.market)
            Text(Strings.Wallet.swapLimitOrderType).tag(OrderType.limit)
          }
            .pickerStyle(SegmentedPickerStyle())
            .resetListHeaderStyle()
            .listRowBackground(Color(.clear))
        ) {
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .navigationTitle(Strings.Wallet.swap)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: { }) {
            Text(Strings.CancelString)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
  }
}

#if DEBUG
struct SwapCryptoView_Previews: PreviewProvider {
  static var previews: some View {
    SwapCryptoView(
      keyringStore: .previewStoreWithWalletCreated,
      ethNetworkStore: .previewStore
    )
    .previewColorSchemes()
  }
}
#endif
