// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct SelectAccountTokenView: View {

  @ObservedObject var store: SelectAccountTokenStore
  @ObservedObject var networkStore: NetworkStore

  @State private var isPresentingNetworkFilter = false
  @Environment(\.presentationMode) @Binding private var presentationMode

  // Asset image sizing
  @ScaledMetric private var assetLogoLength: CGFloat = 40
  private var maxAssetLogoLength: CGFloat = 80
  @ScaledMetric private var networkSymbolLength: CGFloat = 15
  private var maxNetworkSymbolLength: CGFloat = 30

  init(
    store: SelectAccountTokenStore,
    networkStore: NetworkStore
  ) {
    self.store = store
    self.networkStore = networkStore
  }

  var body: some View {
    List {
      if !store.isSetup {
        // Fetching accounts & assets. Typically won't see this.
        ProgressView()
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } else if store.accountSections.isEmpty && !store.isLoadingBalances {
        Text(Strings.Wallet.selectTokenToSendNoTokens)
          .font(.headline.weight(.semibold))
          .foregroundColor(Color(.braveLabel))
          .multilineTextAlignment(.center)
          .frame(maxWidth: .infinity)
          .padding(.vertical, 60)
          .padding(.horizontal, 32)
          .listRowBackground(Color.clear)
      } else {
        accountSections
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .searchable(
      text: $store.query,
      placement: .navigationBarDrawer(displayMode: .always)
    )
    .toolbar {
      ToolbarItemGroup(placement: .cancellationAction) {
        Button {
          presentationMode.dismiss()
        } label: {
          Text(Strings.cancelButtonTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
      ToolbarItemGroup(placement: .bottomBar) {
        networkFilterButton
        Spacer()
        Button {
          store.isHidingZeroBalances.toggle()
        } label: {
          Text(
            store.isHidingZeroBalances
              ? Strings.Wallet.showZeroBalances : Strings.Wallet.hideZeroBalances
          )
          .font(.footnote.weight(.medium))
          .foregroundColor(Color(.braveBlurpleTint))
        }
        .transaction {
          $0.disablesAnimations = true
        }
      }
    }
    .onDisappear {
      store.resetFilters()
    }
  }

  private var networkFilterButton: some View {
    Button {
      self.isPresentingNetworkFilter = true
    } label: {
      Image(braveSystemName: "leo.tune")
        .font(.footnote.weight(.medium))
        .foregroundColor(Color(.braveBlurpleTint))
        .clipShape(Rectangle())
    }
    .sheet(isPresented: $isPresentingNetworkFilter) {
      NavigationView {
        NetworkFilterView(
          networks: store.networkFilters,
          networkStore: networkStore,
          saveAction: { selectedNetworks in
            store.networkFilters = selectedNetworks
          }
        )
      }
      .navigationViewStyle(.stack)
      .onDisappear {
        networkStore.closeNetworkSelectionStore()
      }
    }
  }

  private var accountSections: some View {
    ForEach(store.accountSections) { accountSection in
      Section(
        content: {
          if accountSection.tokenBalances.isEmpty {
            Group {
              if store.isHidingZeroBalances && store.isLoadingBalances {
                // We must fetch balance to confirm >0 before
                // displaying when `isHidingZeroBalances`.
                // Show an asset with shimmer over all content
                // until we know which assets have >0 balance.
                SkeletonLoadingAssetView(
                  assetLogoLength: assetLogoLength,
                  maxAssetLogoLength: maxAssetLogoLength
                )
              } else {
                EmptyView()
              }
            }
            .multilineTextAlignment(.center)
            .frame(maxWidth: .infinity)
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          } else {
            buildAccountSection(accountSection)
          }
        },
        header: {
          WalletListHeaderView {
            CopyAddressHeader(
              displayText: accountSection.account.accountNameDisplay,
              account: accountSection.account,
              btcAccountInfo: accountSection.bitcoinAccountInfo
            )
          }
        }
      )
    }
  }

  private func buildAccountSection(
    _ accountSection: SelectAccountTokenStore.AccountSection
  ) -> some View {
    ForEach(accountSection.tokenBalances) { tokenBalance in
      Button {
        store.didSelect(accountSection.account, tokenBalance.token)
        presentationMode.dismiss()
      } label: {
        if tokenBalance.token.isErc721 || tokenBalance.token.isNft {
          NFTAssetView(
            image: NFTIconView(
              token: tokenBalance.token,
              network: tokenBalance.network,
              url: tokenBalance.nftMetadata?.imageURL,
              shouldShowNetworkIcon: true,
              length: assetLogoLength,
              maxLength: maxAssetLogoLength,
              tokenLogoLength: networkSymbolLength,
              maxTokenLogoLength: maxNetworkSymbolLength
            ),
            title: tokenBalance.token.nftTokenTitle,
            symbol: tokenBalance.token.symbol,
            networkName: tokenBalance.network.chainName,
            quantity: String(Int(tokenBalance.balance ?? 0))
          )
        } else {
          SelectAccountTokenAssetView(
            image: {
              AssetIconView(
                token: tokenBalance.token,
                network: tokenBalance.network,
                shouldShowNetworkIcon: true,
                length: assetLogoLength,
                maxLength: maxAssetLogoLength,
                networkSymbolLength: networkSymbolLength,
                maxNetworkSymbolLength: maxNetworkSymbolLength
              )
            },
            title: tokenBalance.token.name,
            symbol: tokenBalance.token.symbol,
            networkName: tokenBalance.network.chainName,
            quantity: String(format: "%.06f", tokenBalance.balance ?? 0).trimmingTrailingZeros,
            isLoadingBalance: store.isLoadingBalances && tokenBalance.balance == nil,
            price: tokenBalance.price ?? "0",
            isLoadingPrice: (store.isLoadingPrices || store.isLoadingBalances)
              && tokenBalance.price == nil
          )
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
  }
}

struct SelectAccountTokenAssetView<ImageView: View>: View {
  let image: () -> ImageView
  let title: String
  let symbol: String
  let networkName: String
  let quantity: String
  let isLoadingBalance: Bool
  let price: String
  let isLoadingPrice: Bool

  private var priceDisplay: String {
    if isLoadingPrice {  // larger for shimmer effect
      return "0.000"
    }
    if price.isEmpty {
      return "0.0"
    }
    return price
  }

  var body: some View {
    AssetView(
      image: image,
      title: title,
      symbol: symbol,
      networkName: networkName,
      accessoryContent: {
        VStack(alignment: .trailing) {
          Text(verbatim: "\(quantity) \(symbol)")
            .bold()
            .redacted(reason: isLoadingBalance ? .placeholder : [])
            .shimmer(isLoadingBalance)
          Text(priceDisplay)
            .redacted(reason: isLoadingPrice ? .placeholder : [])
            .shimmer(isLoadingPrice)
        }
        .font(.footnote)
        .foregroundColor(Color(.braveLabel))
      }
    )
    .accessibilityLabel("\(title), \(quantity) \(symbol), \(price)")
  }
}

struct SkeletonLoadingAssetView: View {

  @ScaledMetric var assetLogoLength: CGFloat = 40
  var maxAssetLogoLength: CGFloat = 80

  var body: some View {
    SelectAccountTokenAssetView(
      image: {
        Circle()
          .aspectRatio(contentMode: .fit)
          .foregroundColor(Color(.secondaryBraveLabel))
          .frame(
            width: min(assetLogoLength, maxAssetLogoLength),
            height: min(assetLogoLength, maxAssetLogoLength)
          )
          .accessibilityHidden(true)
      },
      title: "Ethereum",
      symbol: "ETH",
      networkName: "Ethereum Mainnet",
      quantity: "0.0",
      isLoadingBalance: false,
      price: "$0.00",
      isLoadingPrice: false
    )
    .accessibilityHidden(true)
    .redacted(reason: .placeholder)
    .shimmer(true)
  }
}
