// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import Foundation
import Preferences
import Strings
import SwiftUI

extension BraveWallet.AssetTimePrice: DataPoint {
  var value: CGFloat {
    Double(price) ?? 0
  }
}

struct AssetDetailHeaderView: View {
  @ObservedObject var assetDetailStore: AssetDetailStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore

  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @Environment(\.openURL) private var openWalletURL
  @Environment(\.colorScheme) private var colourScheme
  @State private var selectedCandle: BraveWallet.AssetTimePrice?

  private var deltaText: some View {
    HStack(spacing: 2) {
      Image(systemName: assetDetailStore.priceIsDown ? "arrow.down" : "arrow.up")
      Text(assetDetailStore.priceDelta)
    }
    .foregroundColor(
      Color(
        assetDetailStore.priceIsDown ? .walletRed : .walletGreen
      )
    )
    .font(.footnote)
  }

  private var emptyData: [BraveWallet.AssetTimePrice] {
    // About 300 points added so it doesn't animate funny
    (0..<300).map { _ in .init(date: Date(), price: "0.0") }
  }

  @ViewBuilder private var tokenInfoView: some View {
    HStack {
      AssetIconView(
        token: assetDetailStore.assetDetailToken,
        network: assetDetailStore.network ?? networkStore.defaultSelectedChain
      )
      if sizeCategory.isAccessibilityCategory {
        VStack(alignment: .leading, spacing: 8) {
          Group {
            Text(assetDetailStore.assetDetailToken.name)
              .fixedSize(horizontal: false, vertical: true)
              .font(.body.weight(.semibold))
              .foregroundColor(Color(.bravePrimary))
            if let chainName = assetDetailStore.network?.chainName {
              Text("\(assetDetailStore.assetDetailToken.symbol) on \(chainName)")
                .fixedSize(horizontal: false, vertical: true)
                .font(.footnote)
                .foregroundColor(Color(.braveLabel))
            }
            Group {
              if let selectedCandle = selectedCandle,
                let formattedString = assetDetailStore.currencyFormatter.formatAsFiat(
                  selectedCandle.value
                )
              {
                Text(formattedString)
              } else {
                Text(assetDetailStore.currencyFormatter.formatAsFiat(assetDetailStore.price) ?? "")
              }
            }
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(.bravePrimary))
          }
          .transaction { transaction in
            transaction.animation = nil
            transaction.disablesAnimations = true
          }
          deltaText
            .redacted(reason: assetDetailStore.isInitialState ? .placeholder : [])
            .shimmer(assetDetailStore.isLoadingPrice)
        }
      } else {
        HStack {
          VStack(alignment: .leading, spacing: 8) {
            Text(assetDetailStore.assetDetailToken.name)
              .fixedSize(horizontal: false, vertical: true)
              .font(.body.weight(.semibold))
              .foregroundColor(Color(.bravePrimary))
            if let chainName = assetDetailStore.network?.chainName {
              Text("\(assetDetailStore.assetDetailToken.symbol) on \(chainName)")
                .fixedSize(horizontal: false, vertical: true)
                .font(.footnote)
                .foregroundColor(Color(.braveLabel))
            }
          }
        }
        Spacer()
        VStack(alignment: .trailing, spacing: 8) {
          Group {
            if let selectedCandle = selectedCandle,
              let formattedString = assetDetailStore.currencyFormatter.formatAsFiat(
                selectedCandle.value
              )
            {
              Text(formattedString)
            } else {
              Text(assetDetailStore.currencyFormatter.formatAsFiat(assetDetailStore.price) ?? "")
            }
          }
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.bravePrimary))
          .transaction { transaction in
            transaction.animation = nil
            transaction.disablesAnimations = true
          }
          deltaText
            .redacted(reason: assetDetailStore.isInitialState ? .placeholder : [])
            .shimmer(assetDetailStore.isLoadingPrice)
        }
      }
    }
  }

  @ViewBuilder private var lineChart: some View {
    VStack(spacing: 0) {
      TimeframeSelector(selectedDateRange: $assetDetailStore.timeframe)
        .padding(.top, 24)
      let data = assetDetailStore.priceHistory.isEmpty ? emptyData : assetDetailStore.priceHistory
      LineChartView(data: data, numberOfColumns: data.count, selectedDataPoint: $selectedCandle) {
        LinearGradient(
          gradient: Gradient(colors: [
            Color(.braveBlurpleTint).opacity(colourScheme == .dark ? 0.5 : 0.2), .clear,
          ]),
          startPoint: .top,
          endPoint: .bottom
        )
        .shimmer(assetDetailStore.isLoadingChart)
      }
      .chartAccessibility(
        title: String.localizedStringWithFormat(
          Strings.Wallet.assetDetailSubtitle,
          assetDetailStore.assetDetailToken.name,
          assetDetailStore.assetDetailToken.symbol
        ),
        dataPoints: data
      )
      .disabled(data.isEmpty)
      .frame(height: 128)
      .padding(.horizontal, -12)
      .animation(.default, value: data)
    }
  }

  var body: some View {
    VStack(spacing: 0) {
      tokenInfoView
        .padding(.bottom, 8)

      lineChart
    }
    .padding()
    .frame(maxWidth: .infinity)
    .background(Color(braveSystemName: .containerBackground))
  }
}

#if DEBUG
struct CurrencyDetailHeaderView_Previews: PreviewProvider {
  static var previews: some View {
    AssetDetailHeaderView(
      assetDetailStore: .previewStore,
      keyringStore: .previewStore,
      networkStore: .previewStore
    )
    .padding(.vertical)
    .previewLayout(.sizeThatFits)
    .previewSizeCategories()
    //    .previewColorSchemes()
  }
}
#endif
