// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct UnavailableBTCBalanceView: View {

  /// BTC Balances for each type
  let btcBalances: [String: [BTCBalanceType: Double]]
  let btcPrice: Double

  // `.sheet()` inside LazyVStack/List sometimes doesn't present;
  // need to pass details to root for presentation brave-ios#8709.
  @Binding var bitcoinBalanceDetails: BitcoinBalanceDetails?

  private func total(for btcBalanceType: BTCBalanceType) -> Double {
    btcBalances.compactMap { $0.value[btcBalanceType] }.reduce(0.0, +)
  }

  var body: some View {
    Button(
      action: {
        self.bitcoinBalanceDetails = .init(
          availableBalance: total(for: .available),
          pendingBalance: total(for: .pending),
          totalBalance: total(for: .total),
          btcPrice: btcPrice
        )
      },
      label: {
        HStack(spacing: 16) {
          ProgressView()
            .progressViewStyle(.braveCircular(size: .small, tint: .braveBlurpleTint))
          Text(Strings.Wallet.btcPendingBalancesBannerDesc)
            .font(.footnote)
            .multilineTextAlignment(.leading)
            .foregroundColor(Color(braveSystemName: .blue50))
          Spacer(minLength: 16)
          Text(Strings.Wallet.viewDetails)
            .font(.footnote.weight(.medium))
            .foregroundColor(Color(braveSystemName: .textInteractive))
        }
        .padding(.vertical, 8)
        .padding(.horizontal, 16)
        .background(Color(braveSystemName: .blue20).cornerRadius(12))
      }
    )
  }
}

#if DEBUG
struct UnavailableBTCBalanceView_Previews: PreviewProvider {
  static var previews: some View {
    UnavailableBTCBalanceView(
      btcBalances: [
        "account.1": [
          .available: 5,
          .pending: 5,
          .total: 10,
        ],
        "account.2": [
          .available: 5,
          .pending: 5,
          .total: 10,
        ],
      ],
      btcPrice: 62_500,
      bitcoinBalanceDetails: .constant(nil)
    )
    .padding()
    .previewLayout(.sizeThatFits)
  }
}
#endif

struct BitcoinBalanceDetails: Equatable {
  let availableBalance: Double
  let pendingBalance: Double
  let totalBalance: Double
  let btcPrice: Double
}

struct BTCBalanceDetailsView: View {

  let details: BitcoinBalanceDetails
  let currencyFormatter: NumberFormatter

  @Environment(\.dismiss) private var dismiss
  @State private var viewHeight: CGFloat = 0

  var body: some View {
    VStack(spacing: 16) {
      HStack {
        Text(Strings.Wallet.detailsButtonTitle)
          .font(.title2.weight(.medium))
          .foregroundColor(Color(braveSystemName: .textPrimary))
        Spacer()
        Button(
          action: {
            dismiss()
          },
          label: {
            Image(braveSystemName: "leo.close")
              .foregroundColor(Color(braveSystemName: .iconDefault))
          }
        )
      }

      VStack(spacing: 0) {
        containerRow(
          title: Strings.Wallet.btcAvailableBalanceTitle,
          description: Strings.Wallet.btcAvailableBalanceDesc,
          value: String(format: "%f", details.availableBalance),
          price: currencyFormatter.string(
            from: .init(value: details.availableBalance * details.btcPrice)
          ) ?? "$0.00"
        )
        DividerLine()
        containerRow(
          title: Strings.Wallet.btcPendingBalanceTitle,
          description: Strings.Wallet.btcPendingBalanceDesc,
          value: String(format: "%f", details.pendingBalance),
          price: currencyFormatter.string(
            from: .init(value: details.pendingBalance * details.btcPrice)
          ) ?? "$0.00"
        )
        DividerLine()
        containerRow(
          title: Strings.Wallet.btcTotalBalanceTitle,
          description: Strings.Wallet.btcTotalBalanceDesc,
          value: String(format: "%f", details.totalBalance),
          price: currencyFormatter.string(
            from: .init(value: details.totalBalance * details.btcPrice)
          ) ?? "$0.00"
        )
        .background(Color(braveSystemName: .containerHighlight))
        .containerShape(Rectangle())
      }
      .frame(maxWidth: .infinity)
      .background(Color(braveSystemName: .containerBackground))
      .overlay {
        ContainerRelativeShape()
          .strokeBorder(Color(braveSystemName: .dividerSubtle))
      }
      .clipShape(ContainerRelativeShape())
      .containerShape(RoundedRectangle(cornerRadius: 12))
    }
    .padding(16)
    .background(Color(braveSystemName: .pageBackground))
    .readSize { size in
      self.viewHeight = size.height
    }
    .presentationDetents([
      .height(viewHeight)
    ])
  }

  private func containerRow(
    title: String,
    description: String,
    value: String,
    price: String
  ) -> some View {
    VStack {
      HStack {
        Text(title)
          .fixedSize(horizontal: false, vertical: true)
        Spacer(minLength: 16)
        Text("\(value) BTC")
          .fixedSize(horizontal: false, vertical: true)
      }
      .font(.callout.weight(.medium))
      .foregroundColor(Color(braveSystemName: .textPrimary))
      HStack(alignment: .top) {
        Text(description)
          .fixedSize(horizontal: false, vertical: true)
        Spacer(minLength: 16)
        Text(price)
          .fixedSize(horizontal: false, vertical: true)
      }
      .font(.caption)
      .foregroundColor(Color(braveSystemName: .textSecondary))
    }
    .padding(16)
  }
}

#if DEBUG
struct BTCBalanceDetailsView_Previews: PreviewProvider {
  static var previews: some View {
    BTCBalanceDetailsView(
      details: .init(
        availableBalance: 10,
        pendingBalance: 5,
        totalBalance: 15,
        btcPrice: 62_500
      ),
      currencyFormatter: .usdCurrencyFormatter
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
