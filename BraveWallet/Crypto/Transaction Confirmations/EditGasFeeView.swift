// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import BraveUI
import struct Shared.Strings

struct EditGasFeeView: View {
  // var estimate: BraveWallet.GasEstimation1559
  
  private enum GasFeeKind: Hashable {
    case low, optimal, high, custom
  }
  
  @State private var gasFeeKind: GasFeeKind = .optimal
  
  var body: some View {
    List {
      Section(
        header: Text(Strings.Wallet.gasFeeDisclaimer)
          .foregroundColor(Color(.secondaryBraveLabel))
          .font(.footnote)
          .resetListHeaderStyle()
          .padding(.vertical)
      ) {
        Picker(selection: $gasFeeKind) {
          Text(Strings.Wallet.gasFeePredefinedLimitLow).tag(GasFeeKind.low)
          Text(Strings.Wallet.gasFeePredefinedLimitOptimal).tag(GasFeeKind.optimal)
          Text(Strings.Wallet.gasFeePredefinedLimitHigh).tag(GasFeeKind.high)
          Text(Strings.Wallet.gasFeeCustomOption).tag(GasFeeKind.custom)
        } label: {
          EmptyView()
        }
        .accentColor(Color(.braveBlurpleTint))
        .pickerStyle(.inline)
        .foregroundColor(Color(.braveLabel))
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      if gasFeeKind == .custom {
        Section(
          header: VStack {
            HStack {
              Text(Strings.Wallet.gasCurrentBaseFee)
              Spacer()
              Text("150 Gwei")
            }
          }
          .font(.headline)
          .foregroundColor(Color(.braveLabel))
          .padding(.top)
          .padding(.bottom, 12)
          .resetListHeaderStyle()
        ) {
          VStack(alignment: .leading, spacing: 4) {
            Text(Strings.Wallet.gasAmountLimit)
              .foregroundColor(Color(.bravePrimary))
              .font(.footnote.weight(.semibold))
            TextField("", text: .constant("2100"))
              .foregroundColor(Color(.braveLabel))
          }
          .padding(.vertical, 6)
          VStack(alignment: .leading, spacing: 4) {
            Text(Strings.Wallet.perGasTipLimit)
              .font(.footnote.weight(.semibold))
              .foregroundColor(Color(.bravePrimary))
            TextField("", text: .constant("2"))
              .foregroundColor(Color(.braveLabel))
          }
          .padding(.vertical, 6)
          VStack(alignment: .leading, spacing: 4) {
            Text(Strings.Wallet.perGasPriceLimit)
              .font(.footnote.weight(.semibold))
              .foregroundColor(Color(.bravePrimary))
            TextField("", text: .constant("150"))
              .foregroundColor(Color(.braveLabel))
          }
          .padding(.vertical, 6)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section {
        VStack {
          Text(Strings.Wallet.maximumGasFee)
            .fontWeight(.regular)
          Text("~$161.24 (0.035731 ETH)")
            .bold()
        }
        .foregroundColor(Color(.braveLabel))
        .font(.headline)
        .padding(.vertical)
        .frame(maxWidth: .infinity)
        .listRowInsets(.zero)
        .listRowBackground(Color(.braveGroupedBackground))
      }
      Section {
        Button(action: { }) {
          Text(Strings.Wallet.saveGasFee)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .frame(maxWidth: .infinity)
        .listRowInsets(.zero)
        .listRowBackground(Color(.braveGroupedBackground))
      }
    }
    .animation(.default, value: gasFeeKind)
    .listStyle(InsetGroupedListStyle())
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.Wallet.maxPriorityFeeTitle)
  }
}

struct EditGasFeeView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      EditGasFeeView()
    }
    .previewColorSchemes()
  }
}
