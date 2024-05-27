// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Foundation
import SDWebImageSwiftUI
import SwiftUI

struct MarketView: View {
  var cryptoStore: CryptoStore
  var keyringStore: KeyringStore
  @ObservedObject var marketStore: MarketStore

  @ScaledMetric private var coinLength: CGFloat = 40
  private let maxCoinSize: CGFloat = 80.0

  @State var allCoingeckoTokens: [BraveWallet.BlockchainToken] = []
  @State private var selectedCoinMarket: BraveWallet.CoinMarket?

  @Environment(\.sizeCategory) private var sizeCategory

  init(cryptoStore: CryptoStore, keyringStore: KeyringStore) {
    self.cryptoStore = cryptoStore
    self.keyringStore = keyringStore
    self.marketStore = cryptoStore.marketStore
  }

  private var emptyState: some View {
    VStack(alignment: .center, spacing: 10) {
      Text(Strings.Wallet.coinMarketEmptyMsg)
        .font(.headline.weight(.semibold))
        .foregroundColor(Color(.braveLabel))
      Button {
        marketStore.update()
      } label: {
        Text(Strings.Wallet.failedToImportAccountErrorMessage)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
    }
    .multilineTextAlignment(.center)
    .frame(maxWidth: .infinity)
    .padding(.vertical, 60)
    .padding(.horizontal, 32)
  }

  @ViewBuilder private var loadingTokenPlaceholder: some View {
    Circle()
      .aspectRatio(contentMode: .fit)
      .foregroundColor(Color(.secondaryBraveLabel))
      .frame(width: min(coinLength, maxCoinSize), height: min(coinLength, maxCoinSize))
      .accessibilityHidden(true)
    VStack(alignment: .leading, spacing: 4) {
      Text("Token name")
        .foregroundColor(Color(.braveLabel))
      Text("Token symbol")
        .foregroundColor(Color(.secondaryBraveLabel))
    }
  }

  @ViewBuilder private func tokenInfoView(_ coinMarket: BraveWallet.CoinMarket) -> some View {
    WebImage(url: URL(string: coinMarket.image))
      .resizable()
      .placeholder {
        BlockieBackground(seed: coinMarket.id)
          .blur(radius: 8, opaque: true)
          .clipShape(Circle())
          .overlay(
            Text(coinMarket.symbol.first?.uppercased() ?? "")
              .font(.system(size: coinLength / 2, weight: .bold, design: .rounded))
              .foregroundColor(.white)
              .shadow(color: .black.opacity(0.3), radius: 2, x: 0, y: 1)
          )
      }
      .indicator(.activity)
      .aspectRatio(contentMode: .fit)
      .frame(width: min(coinLength, maxCoinSize), height: min(coinLength, maxCoinSize))
      .accessibilityHidden(true)
    VStack(alignment: .leading, spacing: 4) {
      Text(coinMarket.name)
        .foregroundColor(Color(.braveLabel))
        .fontWeight(.semibold)
      Text(coinMarket.symbol.uppercased())
        .foregroundColor(Color(.secondaryBraveLabel))
    }
    .font(.footnote)
  }

  private var loadingView: some View {
    ForEach(0...10, id: \.self) { _ in
      HStack {
        if sizeCategory.isAccessibilityCategory {
          VStack(alignment: .leading, spacing: 8) {
            loadingTokenPlaceholder
          }
        } else {
          HStack(spacing: 8) {
            loadingTokenPlaceholder
          }
        }
        Spacer()
        VStack(alignment: .trailing, spacing: 4) {
          Text("$0.0000")
            .foregroundColor(Color(.braveLabel))
            .padding(.vertical, 8)
          Text("0.00%")
        }
      }
      .redacted(reason: .placeholder)
      .shimmer(true)
    }
  }

  var body: some View {
    List {
      Section {
        Group {
          if marketStore.isLoading && marketStore.coins.isEmpty {
            loadingView
          } else if marketStore.coins.isEmpty {
            emptyState
              .listRowBackground(Color.clear)
          } else {
            ForEach(marketStore.coins, id: \.uniqueId) { coinMarket in
              Button {
                selectedCoinMarket = coinMarket
              } label: {
                HStack {
                  if sizeCategory.isAccessibilityCategory {
                    VStack(alignment: .leading, spacing: 8) {
                      tokenInfoView(coinMarket)
                    }
                  } else {
                    HStack(spacing: 8) {
                      tokenInfoView(coinMarket)
                    }
                  }

                  Spacer()

                  VStack(alignment: .trailing, spacing: 4) {
                    Text(
                      marketStore.priceFormatter.coinMarketPriceString(
                        from: coinMarket.currentPrice
                      ) ?? "$0.00"
                    )
                    .font(.footnote)
                    .foregroundColor(Color(.braveLabel))
                    .padding(.vertical, 8)
                    HStack {
                      Image(
                        systemName: coinMarket.priceChangePercentage24h > 0
                          ? "arrow.up" : "arrow.down"
                      )
                      .imageScale(.small)
                      .accessibilityHidden(true)
                      Text(
                        "\(marketStore.priceChangeFormatter.string(from: NSNumber(value: abs(coinMarket.priceChangePercentage24h / 100))) ?? "")"
                      )
                      .font(.caption)
                    }
                    .foregroundColor(
                      coinMarket.priceChangePercentage24h > 0
                        ? Color(.walletGreen) : Color(.walletRed)
                    )
                  }
                }
              }
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(.braveGroupedBackground))
    .background(
      NavigationLink(
        isActive: Binding(
          get: { selectedCoinMarket != nil },
          set: { if !$0 { selectedCoinMarket = nil } }
        ),
        destination: {
          if let coinMarket = selectedCoinMarket {
            AssetDetailView(
              assetDetailStore: cryptoStore.assetDetailStore(for: .coinMarket(coinMarket)),
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore
            )
            .onDisappear {
              cryptoStore.closeAssetDetailStore(for: .coinMarket(coinMarket))
            }
          }
        },
        label: {
          EmptyView()
        }
      )
    )
    .onAppear {
      marketStore.update()
    }
  }
}

#if DEBUG
struct MarketView_Previews: PreviewProvider {
  static var previews: some View {
    MarketView(
      cryptoStore: .previewStore,
      keyringStore: .previewStore
    )
  }
}
#endif
