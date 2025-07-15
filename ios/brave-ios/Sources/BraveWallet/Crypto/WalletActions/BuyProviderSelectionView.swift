// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import BraveUI
import DesignSystem
import OrderedCollections
import SwiftUI

struct BuyProviderSelectionView: View {
  @ObservedObject var buyTokenStore: BuyTokenStore
  @ObservedObject var keyringStore: KeyringStore
  let sortedQuotes: [BraveWallet.MeldCryptoQuote]
  let providers: [BraveWallet.MeldServiceProvider]

  @Environment(\.openURL) private var openWalletURL

  @ScaledMetric private var iconSize = 40.0
  private let maxIconSize: CGFloat = 80.0

  var body: some View {
    Group {
      if sortedQuotes.isEmpty {
        VStack {
          Image(braveSystemName: "leo.info.outline")
            .resizable()
            .frame(width: 24, height: 24)
            .padding(16)
            .foregroundColor(Color(braveSystemName: .iconDefault))
            .background(
              Color(
                braveSystemName: .pageBackground
              ).clipShape(.circle)
            )
          Text(
            String.localizedStringWithFormat(
              Strings.Wallet.providerListNoProviderTitle,
              buyTokenStore.selectedBuyToken.displaySymbol
            )
          )
          .fontWeight(.semibold)
          .foregroundColor(Color(braveSystemName: .textPrimary))
          Text(Strings.Wallet.providerListNoProviderDescription)
            .font(.footnote)
            .foregroundColor(Color(braveSystemName: .textTertiary))
        }
      } else {
        List {
          Section(
            header: WalletListHeaderView(
              title: Text(Strings.Wallet.providerSelectionSectionHeader)
            )
          ) {
            ForEach(sortedQuotes.indices, id: \.self) { index in
              if let quote = sortedQuotes[safe: index],
                let provider = providers.first(where: {
                  $0.serviceProvider == quote.serviceProvider
                })
              {
                ProviderView(
                  buyTokenStore: buyTokenStore,
                  quote: quote,
                  provider: provider,
                  isBestOption: index == 0
                )
              }
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        }
        .listStyle(InsetGroupedListStyle())
        .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      }
    }
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.Wallet.providerSelectionScreenTitle)
  }
}

private struct ProviderView: View {
  let buyTokenStore: BuyTokenStore
  let quote: BraveWallet.MeldCryptoQuote
  let provider: BraveWallet.MeldServiceProvider
  let isBestOption: Bool

  @State private var isExpanded: Bool = false
  @State private var isFetchingBuyUrl: Bool = false
  @ScaledMetric private var iconSize = 40.0
  @Environment(\.openURL) private var openWalletURL
  private let maxIconSize: CGFloat = 80.0

  init(
    buyTokenStore: BuyTokenStore,
    quote: BraveWallet.MeldCryptoQuote,
    provider: BraveWallet.MeldServiceProvider,
    isBestOption: Bool
  ) {
    self.buyTokenStore = buyTokenStore
    self.quote = quote
    self.provider = provider
    self.isBestOption = isBestOption
    _isExpanded = .init(initialValue: isBestOption)
  }

  var body: some View {
    VStack {
      Button {
        isExpanded.toggle()
      } label: {
        HStack {
          if let urlString = provider.logoImages?.lightShortUrl,
            let url = URL(string: urlString)
          {
            WebImageReader(url: url) { image in
              if let image = image {
                Image(uiImage: image)
                  .resizable()
                  .aspectRatio(contentMode: .fit)
                  .frame(width: min(iconSize, maxIconSize), height: min(iconSize, maxIconSize))
              } else {
                Rectangle()
                  .foregroundColor(Color(.tertiaryBraveGroupedBackground))
                  .frame(width: min(iconSize, maxIconSize), height: min(iconSize, maxIconSize))
                  .shimmer(true)
              }
            }
          } else {
            Rectangle()
              .foregroundColor(Color(.tertiaryBraveGroupedBackground))
              .frame(width: min(iconSize, maxIconSize), height: min(iconSize, maxIconSize))
              .shimmer(true)
          }
          VStack(alignment: .leading, spacing: 4) {
            Text(provider.name ?? "")
              .foregroundColor(Color(.bravePrimary))
              .font(.headline)
              .multilineTextAlignment(.leading)
            if let sourceAmount = quote.sourceAmount,
              let sourceCurrencyCode = quote.sourceCurrencyCode,
              let destinationAmount = quote.destinationAmount,
              let destinationAmountBDouble = BDouble(destinationAmount),
              let destinationCurrencyCode = quote.destinationCurrencyCode
            {
              Text(
                "\(sourceAmount) \(sourceCurrencyCode) ≈ \(destinationAmountBDouble.decimalDescription.trimmingTrailingZeros) \(destinationCurrencyCode)"
              )
              .foregroundColor(Color(.bravePrimary))
              .font(.footnote)
              .multilineTextAlignment(.leading)
            }
          }
          Spacer()
          if isBestOption {
            Label(Strings.Wallet.providerListBestOption, braveSystemImage: "leo.thumb.up")
              .padding(4)
              .font(.caption2.weight(.semibold))
              .foregroundColor(Color(braveSystemName: .green60))
              .background(Color(braveSystemName: .green10))
              .cornerRadius(4)
          }
          Image(braveSystemName: isExpanded ? "leo.carat.up" : "leo.carat.down")
            .animation(.default, value: isExpanded)
        }
        .padding(.vertical, 10)
      }
      .modifier(WalletButtonStyleModifier())
      if isExpanded {
        VStack {
          Group {
            if let exchangeRate = quote.exchangeRate,
              let exchangeRateBDouble = BDouble(exchangeRate),
              let sourceCurrencyCode = quote.sourceCurrencyCode,
              let destinationCurrencyCode = quote.destinationCurrencyCode
            {
              HStack {
                Text(Strings.Wallet.providerListExchangeRateWithFees)
                Spacer()
                Text(
                  "≈ \(exchangeRateBDouble.decimalDescription.trimmingTrailingZeros) \(sourceCurrencyCode) / \(destinationCurrencyCode)"
                )
                .multilineTextAlignment(.trailing)
              }
            }
            if let sourceAmountWithoutFee = quote.sourceAmountWithoutFee,
              let sourceAmountWithoutFeeBDouble = BDouble(sourceAmountWithoutFee),
              let sourceCurrencyCode = quote.sourceCurrencyCode
            {
              HStack {
                Text(Strings.Wallet.providerListPrice) + Text(" \(sourceCurrencyCode)")
                Spacer()
                Text(
                  "≈ \(sourceAmountWithoutFeeBDouble.decimalDescription.trimmingTrailingZeros) \(sourceCurrencyCode)"
                )
                .multilineTextAlignment(.trailing)
              }
            }
            if let totalFee = quote.totalFee,
              let totalFeeBDouble = BDouble(totalFee),
              let sourceCurrencyCode = quote.sourceCurrencyCode
            {
              HStack {
                Text(Strings.Wallet.providerListFees)
                Spacer()
                Text(
                  "\(totalFeeBDouble.decimalDescription.trimmingTrailingZeros) \(sourceCurrencyCode)"
                )
                .multilineTextAlignment(.trailing)
              }
            }
            if let total = quote.sourceAmount,
              let totalBDouble = BDouble(total),
              let sourceCurrencyCode = quote.sourceCurrencyCode
            {
              HStack {
                Text(Strings.Wallet.providerListTotal)
                Spacer()
                Text(
                  "\(totalBDouble.decimalDescription.trimmingTrailingZeros) \(sourceCurrencyCode)"
                )
                .multilineTextAlignment(.trailing)
              }
            }
          }
          .font(.footnote)
          WalletLoadingButton(
            isLoading: isFetchingBuyUrl
          ) {
            Task {
              isFetchingBuyUrl = true
              if let url = await buyTokenStore.fetchBuyUrl(
                provider: provider
              ) {
                isFetchingBuyUrl = false
                openWalletURL(url)
              }
              isFetchingBuyUrl = false
            }
          } label: {
            HStack {
              Text(
                String.localizedStringWithFormat(
                  Strings.Wallet.providerListBuyWith,
                  provider.name ?? ""
                )
              )
              Image(braveSystemName: "leo.launch")
            }
          }
          .buttonStyle(BraveFilledButtonStyle(size: .normal))
          .frame(maxWidth: .infinity)
        }
      }
    }
  }
}

#if DEBUG
struct BuyProviderSelectionView_Previews: PreviewProvider {
  static var previews: some View {
    BuyProviderSelectionView(
      buyTokenStore: .previewStore,
      keyringStore: .previewStore,
      sortedQuotes: [],
      providers: []
    )
  }
}
#endif
