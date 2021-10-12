/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import BraveUI
import struct Shared.Strings

struct AssetDetailHeaderView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  var currency: Currency
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @State private var selectedDateRange: BraveWallet.AssetPriceTimeframe = .oneDay
  @State private var selectedCandle: Candle?
  
  var data: [Candle] {
    switch selectedDateRange {
    case .oneDay:
      return ([10, 20, 30, 20, 10, 40, 50, 80, 100] as [CGFloat]).map(Candle.init)
    case .live:
      return ([10, 20, 30, 20, 10, 40, 50, 80, 100] as [CGFloat]).map(Candle.init).reversed()
    case .oneWeek:
      return ([10, 20, 30, 20, 10] as [CGFloat]).map(Candle.init)
    case .oneMonth:
      return ([10, 20, 30, 20, 10, 40, 50, 80, 100, 200, 100, 120] as [CGFloat]).map(Candle.init)
    case .threeMonths:
      return ([10, 20, 30, 20, 10, 40, 50, 80, 100] as [CGFloat]).map(Candle.init)
    case .oneYear:
      return ([10, 20, 30, 20, 10, 40, 50, 80, 100] as [CGFloat]).map(Candle.init)
    case .all:
      return ([10, 20, 30, 20, 10, 40, 50, 80, 100] as [CGFloat]).map(Candle.init)
    @unknown default:
      return [10, 20, 30, 20, 10, 40, 50, 80, 100].map(Candle.init)
    }
  }
  
  private var deltaText: some View {
    HStack(spacing: 2) {
      Image(systemName: "arrow.up")
      Text(verbatim: "0.46%")
    }
    .foregroundColor(.white)
    .font(.caption2)
    .padding(5)
    .background(
      Color(.braveSuccessLabel)
        .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
    )
  }
  
  var body: some View {
    VStack(spacing: 0) {
      VStack(alignment: .leading) {
        if sizeCategory.isAccessibilityCategory {
          VStack(alignment: .leading) {
            HStack {
              NetworkPicker(
                networks: networkStore.ethereumChains,
                selectedNetwork: networkStore.selectedChainBinding
              )
              if horizontalSizeClass == .regular {
                Spacer()
                DateRangeView(selectedRange: $selectedDateRange)
                  .padding(6)
                  .overlay(
                    RoundedRectangle(cornerRadius: 8, style: .continuous)
                      .strokeBorder(Color(.secondaryButtonTint))
                  )
              }
            }
            HStack {
              Circle().frame(width: 40, height: 40)
              Text(currency.name)
                .fixedSize(horizontal: false, vertical: true)
                .font(.title3.weight(.semibold))
            }
          }
          .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          HStack {
            Circle().frame(width: 40, height: 40)
            Text(currency.name)
              .fixedSize(horizontal: false, vertical: true)
              .font(.title3.weight(.semibold))
            NetworkPicker(
              networks: networkStore.ethereumChains,
              selectedNetwork: networkStore.selectedChainBinding
            )
            if horizontalSizeClass == .regular {
              Spacer()
              DateRangeView(selectedRange: $selectedDateRange)
                .padding(6)
                .overlay(
                  RoundedRectangle(cornerRadius: 10, style: .continuous)
                    .strokeBorder(Color(.secondaryButtonTint))
                )
            }
          }
        }
        Text(String.localizedStringWithFormat(Strings.Wallet.assetDetailSubtitle,
                                              currency.name, currency.symbol))
          .font(.footnote)
          .foregroundColor(Color(.secondaryBraveLabel))
        HStack {
          Text(verbatim: "$1,812.31")
            .font(.title2.bold())
          deltaText
          Spacer()
        }
        Text(verbatim: "0.03265 BTC")
          .font(.footnote)
          .foregroundColor(Color(.secondaryBraveLabel))
        LineChartView(data: data, numberOfColumns: 12, selectedDataPoint: $selectedCandle) {
          Color(.braveSuccessLabel)
        }
        .frame(height: 128)
        .padding(.horizontal, -16)
        .animation(.default, value: data)
        if horizontalSizeClass == .compact {
          DateRangeView(selectedRange: $selectedDateRange)
        }
      }
      .padding(16)
      Divider()
        .padding(.bottom)
      HStack {
        Button(action: { }) {
          Text(Strings.Wallet.buy)
        }
        Button(action: { }) {
          Text(Strings.Wallet.send)
        }
        Button(action: { }) {
          Text(Strings.Wallet.swap)
        }
      }
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
    }
  }
}

#if DEBUG
struct CurrencyDetailHeaderView_Previews: PreviewProvider {
  static var previews: some View {
    AssetDetailHeaderView(
      keyringStore: .previewStore,
      networkStore: .previewStore,
      currency: .init(image: UIImage(), name: "Basic Attention Token", symbol: "BAT", cost: 2.00)
    )
    .padding(.vertical)
    .previewLayout(.sizeThatFits)
    .previewSizeCategories()
//    .previewColorSchemes()
  }
}
#endif
