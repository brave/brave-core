/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import DesignSystem
import Strings
import Preferences

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
  var onAccountCreationNeeded: (_ savedDestination: BuySendSwapDestination) -> Void

  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @Environment(\.openURL) private var openWalletURL
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
  
  @ViewBuilder private var actionButtonsContainer: some View {
    if assetDetailStore.isBuySupported && assetDetailStore.isSwapSupported {
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
    if case let .blockchainToken(token) = assetDetailStore.assetDetailType, token.isAuroraSupportedToken {
      auroraBridgeButton
    }
  }
  
  @ViewBuilder var buySendSwapButtonsContainer: some View {
    HStack {
      if assetDetailStore.isBuySupported {
        Button(
          action: {
            let destination = BuySendSwapDestination(
              kind: .buy,
              initialToken: assetDetailStore.assetDetailToken
            )
            if assetDetailStore.accounts.isEmpty {
              onAccountCreationNeeded(destination)
            } else {
              buySendSwapDestination = destination
            }
          }
        ) {
          Text(Strings.Wallet.buy)
        }
      }
      if assetDetailStore.isSendSupported {
        Button(
          action: {
            let destination = BuySendSwapDestination(
              kind: .send,
              initialToken: assetDetailStore.assetDetailToken
            )
            if assetDetailStore.accounts.isEmpty {
              onAccountCreationNeeded(destination)
            } else {
              buySendSwapDestination = destination
            }
          }
        ) {
          Text(Strings.Wallet.send)
        }
      }
      if assetDetailStore.isSwapSupported && assetDetailStore.assetDetailToken.isFungibleToken {
        Button(
          action: {
            let destination = BuySendSwapDestination(
              kind: .swap,
              initialToken: assetDetailStore.assetDetailToken
            )
            if assetDetailStore.accounts.isEmpty {
              onAccountCreationNeeded(destination)
            } else {
              buySendSwapDestination = destination
            }
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
            openWalletURL(link)
          }
        }
      }
    ) {
      Text(Strings.Wallet.auroraBridgeButtonTitle)
    }
    .buttonStyle(BraveFilledButtonStyle(size: .normal))
  }
  
  @ViewBuilder private var tokenImageNameAndNetwork: some View {
    AssetIconView(token: assetDetailStore.assetDetailToken, network: assetDetailStore.network ?? networkStore.defaultSelectedChain)
    VStack(alignment: .leading) {
      Text(assetDetailStore.assetDetailToken.name)
        .fixedSize(horizontal: false, vertical: true)
        .font(.title3.weight(.semibold))
      if let chainName = assetDetailStore.network?.chainName {
        Text(chainName)
          .fixedSize(horizontal: false, vertical: true)
          .font(.caption)
      }
    }
  }

  var body: some View {
    VStack(alignment: assetDetailStore.assetDetailToken.isFungibleToken ? .center : .leading, spacing: 0) {
      if assetDetailStore.assetDetailToken.isFungibleToken {
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
                tokenImageNameAndNetwork
              }
            }
            .frame(maxWidth: .infinity, alignment: .leading)
          } else {
            HStack {
              tokenImageNameAndNetwork
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
              assetDetailStore.assetDetailToken.name,
              assetDetailStore.assetDetailToken.symbol)
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
              assetDetailStore.assetDetailToken.name,
              assetDetailStore.assetDetailToken.symbol),
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
      } else {
        HStack {
          AssetIconView(token: assetDetailStore.assetDetailToken, network: networkStore.defaultSelectedChain)
          Text(assetDetailStore.assetDetailToken.nftTokenTitle)
            .fixedSize(horizontal: false, vertical: true)
            .font(.title3.weight(.semibold))
          Spacer()
        }
        .padding(16)
      }
      if assetDetailStore.assetDetailToken.isFungibleToken {
        Divider()
          .padding(.bottom)
      }
      actionButtonsContainer
        .padding(.horizontal, assetDetailStore.assetDetailToken.isFungibleToken ? 0 : 16)
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
      isShowingBridgeAlert: .constant(false),
      onAccountCreationNeeded: { _ in }
    )
    .padding(.vertical)
    .previewLayout(.sizeThatFits)
    .previewSizeCategories()
    //    .previewColorSchemes()
  }
}
#endif

private extension BraveWallet.BlockchainToken {
  var isFungibleToken: Bool {
    return !isErc721 && !isNft
  }
}
