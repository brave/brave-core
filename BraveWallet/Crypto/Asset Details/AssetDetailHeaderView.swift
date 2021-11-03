/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import BraveUI
import struct Shared.Strings

extension BraveWallet.AssetTimePrice: DataPoint {
  var value: CGFloat {
    Double(price) ?? 0
  }
}

struct AssetDetailHeaderView: View {
  @ObservedObject var assetDetailStore: AssetDetailStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @Binding var buySendSwapDestination: BuySendSwapDestination?
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @State private var selectedCandle: BraveWallet.AssetTimePrice?
  
  private var deltaText: some View {
    HStack(spacing: 2) {
      Image(systemName: assetDetailStore.priceIsDown ? "arrow.down" : "arrow.up")
      Text(assetDetailStore.priceDelta)
    }
    .foregroundColor(.white)
    .font(.caption2)
    .padding(5)
    .background(
      Color(
        assetDetailStore.priceIsDown ? .walletRed : .walletGreen
      )
        .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
    )
  }
  
  private var emptyData: [BraveWallet.AssetTimePrice] {
    // About 300 points added so it doesn't animate funny
    (0..<300).map { _ in .init(date: Date(), price: "0.0") }
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
                DateRangeView(selectedRange: $assetDetailStore.timeframe)
                  .padding(6)
                  .overlay(
                    RoundedRectangle(cornerRadius: 8, style: .continuous)
                      .strokeBorder(Color(.secondaryButtonTint))
                  )
              }
            }
            HStack {
              AssetIconView(token: assetDetailStore.token)
              Text(assetDetailStore.token.name)
                .fixedSize(horizontal: false, vertical: true)
                .font(.title3.weight(.semibold))
            }
          }
          .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          HStack {
            AssetIconView(token: assetDetailStore.token)
            Text(assetDetailStore.token.name)
              .fixedSize(horizontal: false, vertical: true)
              .font(.title3.weight(.semibold))
            NetworkPicker(
              networks: networkStore.ethereumChains,
              selectedNetwork: networkStore.selectedChainBinding
            )
            if horizontalSizeClass == .regular {
              Spacer()
              DateRangeView(selectedRange: $assetDetailStore.timeframe)
                .padding(6)
                .overlay(
                  RoundedRectangle(cornerRadius: 10, style: .continuous)
                    .strokeBorder(Color(.secondaryButtonTint))
                )
            }
          }
        }
        Text(String.localizedStringWithFormat(Strings.Wallet.assetDetailSubtitle,
                                              assetDetailStore.token.name,
                                              assetDetailStore.token.symbol))
          .font(.footnote)
          .foregroundColor(Color(.secondaryBraveLabel))
        VStack(alignment: .leading) {
          HStack {
            Group {
              if let selectedCandle = selectedCandle,
                 let formattedString = AssetDetailStore.priceFormatter.string(from: NSNumber(value: selectedCandle.value)) {
                Text(formattedString)
              } else {
                Text(assetDetailStore.price)
              }
            }
            .font(.title2.bold())
            deltaText
            Spacer()
          }
          Text(assetDetailStore.btcRatio)
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
        .redacted(reason: assetDetailStore.isInitialState ? .placeholder : [])
        .shimmer(assetDetailStore.isLoadingPrice)
        let data = assetDetailStore.priceHistory.isEmpty ? emptyData : assetDetailStore.priceHistory
        LineChartView(data: data, numberOfColumns: data.count, selectedDataPoint: $selectedCandle) {
          Color(.walletGreen)
            .shimmer(assetDetailStore.isLoadingChart)
        }
        .disabled(data.isEmpty)
        .frame(height: 128)
        .padding(.horizontal, -16)
        .animation(.default, value: data)
        if horizontalSizeClass == .compact {
          DateRangeView(selectedRange: $assetDetailStore.timeframe)
        }
      }
      .padding(16)
      Divider()
        .padding(.bottom)
      HStack {
        Button(action: { buySendSwapDestination = .buy }) {
          Text(Strings.Wallet.buy)
        }
        Button(action: { buySendSwapDestination = .send }) {
          Text(Strings.Wallet.send)
        }
        Button(action: { buySendSwapDestination = .swap }) {
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
      assetDetailStore: .previewStore,
      keyringStore: .previewStore,
      networkStore: .previewStore,
      buySendSwapDestination: .constant(nil)
    )
    .padding(.vertical)
    .previewLayout(.sizeThatFits)
    .previewSizeCategories()
//    .previewColorSchemes()
  }
}
#endif
