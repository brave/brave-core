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

  private func total(for btcBalanceType: BTCBalanceType) -> Double {
    btcBalances.compactMap { $0.value[btcBalanceType] }.reduce(0.0, +)
  }

  @State private var isShowingBalanceDetails: Bool = false

  var body: some View {
    Button(
      action: { self.isShowingBalanceDetails = true },
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
    .sheet(isPresented: $isShowingBalanceDetails) {
      BTCBalanceDetailsView(
        availableBalance: total(for: .available),
        pendingBalance: total(for: .pending),
        totalBalance: total(for: .total),
        btcPrice: btcPrice,
        currencyFormatter: .usdCurrencyFormatter
      )
    }
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
      btcPrice: 62_500
    )
    .padding()
    .previewLayout(.sizeThatFits)
  }
}
#endif

struct BTCBalanceDetailsView: View {

  let availableBalance: Double
  let pendingBalance: Double
  let totalBalance: Double
  let btcPrice: Double
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
          value: String(format: "%f", availableBalance),
          price: currencyFormatter.string(from: .init(value: availableBalance * btcPrice))
            ?? "$0.00"
        )
        DividerLine()
        containerRow(
          title: Strings.Wallet.btcPendingBalanceTitle,
          description: Strings.Wallet.btcPendingBalanceDesc,
          value: String(format: "%f", pendingBalance),
          price: currencyFormatter.string(from: .init(value: pendingBalance * btcPrice)) ?? "$0.00"
        )
        DividerLine()
        containerRow(
          title: Strings.Wallet.btcTotalBalanceTitle,
          description: Strings.Wallet.btcTotalBalanceDesc,
          value: String(format: "%f", totalBalance),
          price: currencyFormatter.string(from: .init(value: totalBalance * btcPrice)) ?? "$0.00"
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
    .osAvailabilityModifiers({ view in
      if #available(iOS 16, *) {
        view
          .presentationDetents([
            .height(viewHeight)
          ])
      } else {
        view
      }
    })
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
      availableBalance: 10,
      pendingBalance: 5,
      totalBalance: 15,
      btcPrice: 62_500,
      currencyFormatter: .usdCurrencyFormatter
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
