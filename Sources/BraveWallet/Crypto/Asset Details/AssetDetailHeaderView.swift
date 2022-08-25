/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import DesignSystem
import Strings
import BraveShared

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
  @Binding var isShowingBridgeAlert: Bool

  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @Environment(\.openWalletURLAction) private var openWalletURL
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
  
  private var isBuySupported: Bool {
    assetDetailStore.isBuySupported
    && WalletConstants.supportedBuyWithWyreNetworkChainIds.contains(networkStore.selectedChainId)
  }
  
  @ViewBuilder private var actionButtonsContainer: some View {
    if isBuySupported && networkStore.isSwapSupported {
      VStack {
        actionButtons
      }
    } else {
      HStack {
        actionButtons
      }
    }
  }
  
  @ViewBuilder private var actionButtons: some View {
    buySendSwapButtonsContainer
    if assetDetailStore.token.isAuroraSupportedToken {
      auroraBridgeButton
    }
  }
  
  @ViewBuilder var buySendSwapButtonsContainer: some View {
    HStack {
      if isBuySupported {
        Button(
          action: {
            buySendSwapDestination = BuySendSwapDestination(
              kind: .buy,
              initialToken: assetDetailStore.token
            )
          }
        ) {
          Text(Strings.Wallet.buy)
        }
      }
      Button(
        action: {
          buySendSwapDestination = BuySendSwapDestination(
            kind: .send,
            initialToken: assetDetailStore.token
          )
        }
      ) {
        Text(Strings.Wallet.send)
      }
      if networkStore.isSwapSupported {
        Button(
          action: {
            buySendSwapDestination = BuySendSwapDestination(
              kind: .swap,
              initialToken: assetDetailStore.token
            )
          }
        ) {
          Text(Strings.Wallet.swap)
        }
      }
    }
    .buttonStyle(BraveFilledButtonStyle(size: .normal))
  }
  
  @ViewBuilder var auroraBridgeButton: some View {
    Button(
      action: {
        if Preferences.Wallet.showAuroraPopup.value {
          isShowingBridgeAlert = true
        } else {
          if let link = WalletConstants.auroraBridgeLink {
            openWalletURL?(link)
          }
        }
      }
    ) {
      Text(Strings.Wallet.auroraBridgeButtonTitle)
    }
    .buttonStyle(BraveFilledButtonStyle(size: .normal))
  }

  var body: some View {
    VStack(spacing: 0) {
      VStack(alignment: .leading) {
        if sizeCategory.isAccessibilityCategory {
          VStack(alignment: .leading) {
            if horizontalSizeClass == .regular {
              DateRangeView(selectedRange: $assetDetailStore.timeframe)
                .padding(6)
                .overlay(
                  RoundedRectangle(cornerRadius: 8, style: .continuous)
                    .strokeBorder(Color(.secondaryButtonTint))
                )
            }
            HStack {
              AssetIconView(token: assetDetailStore.token, network: networkStore.selectedChain)
              Text(assetDetailStore.token.name)
                .fixedSize(horizontal: false, vertical: true)
                .font(.title3.weight(.semibold))
            }
          }
          .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          HStack {
            AssetIconView(token: assetDetailStore.token, network: networkStore.selectedChain)
            Text(assetDetailStore.token.name)
              .fixedSize(horizontal: false, vertical: true)
              .font(.title3.weight(.semibold))
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
        Text(
          String.localizedStringWithFormat(
            Strings.Wallet.assetDetailSubtitle,
            assetDetailStore.token.name,
            assetDetailStore.token.symbol)
        )
        .font(.footnote)
        .foregroundColor(Color(.secondaryBraveLabel))
        VStack(alignment: .leading) {
          HStack {
            Group {
              if let selectedCandle = selectedCandle,
                let formattedString = assetDetailStore.currencyFormatter.string(from: NSNumber(value: selectedCandle.value)) {
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
        .chartAccessibility(
          title: String.localizedStringWithFormat(
            Strings.Wallet.assetDetailSubtitle,
            assetDetailStore.token.name,
            assetDetailStore.token.symbol),
          dataPoints: data
        )
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
      actionButtonsContainer
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
      buySendSwapDestination: .constant(nil),
      isShowingBridgeAlert: .constant(false)
    )
    .padding(.vertical)
    .previewLayout(.sizeThatFits)
    .previewSizeCategories()
    //    .previewColorSchemes()
  }
}
#endif
