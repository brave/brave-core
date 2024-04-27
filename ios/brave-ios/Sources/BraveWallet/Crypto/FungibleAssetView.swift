// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import SwiftUI
import BraveCore

// View used to display `FungibleAssetView` as a button.
struct FungibleAssetButton: View {
  let asset: AssetViewModel
  /// If a container should be shown to around unavailable balance banner
  /// for Bitcoin asset row (when there is a pending balance)
  let shouldShowContainerForBitcoin: Bool
  let currencyFormatter: NumberFormatter
  @Binding var bitcoinBalanceDetails: BitcoinBalanceDetails?
  let action: (BraveWallet.BlockchainToken) -> Void
  
  private var showUnavailableBTCBalanceBanner: Bool {
    guard asset.token.coin == .btc else {
      return false
    }
    let sumOfPendingBalances = asset.btcBalances
      .compactMap({ $0.value[.pending] })
      .reduce(0.0, +)
    return sumOfPendingBalances != 0
  }
  
  var body: some View {
    // Avoid button inside a button for `UnavailableBTCBalanceView`
    if showUnavailableBTCBalanceBanner {
      VStack(spacing: 0) {
        assetViewButton
        
        UnavailableBTCBalanceView(
          btcBalances: asset.btcBalances,
          btcPrice: Double(asset.price) ?? 0,
          bitcoinBalanceDetails: $bitcoinBalanceDetails
        )
      }
      .padding()
      .overlay {
        if shouldShowContainerForBitcoin {
          ContainerRelativeShape()
            .strokeBorder(Color(braveSystemName: .dividerSubtle))
        }
      }
      .containerShape(RoundedRectangle(cornerRadius: 12))
    } else {
      assetViewButton
        .padding(.horizontal)
    }
  }
  
  private var assetViewButton: some View {
    Button {
      action(asset.token)
    } label: {
      FungibleAssetView(
        image: AssetIconView(
          token: asset.token,
          network: asset.network,
          shouldShowNetworkIcon: true
        ),
        title: asset.token.name,
        symbol: asset.token.symbol,
        networkName: asset.network.chainName,
        fiat: asset.fiatAmount(currencyFormatter: currencyFormatter),
        balance: asset.quantity,
        shouldHideBalance: true,
        btcBalances: asset.btcBalances
      )
    }
  }
}

struct FungibleAssetView: View {
  var image: AssetIconView
  var title: String
  var symbol: String
  let networkName: String
  var fiat: String
  var balance: String
  let shouldHideBalance: Bool
  /// All BTC balance types (only there is a pending balance to indicate to user)
  let btcBalances: [String: [BTCBalanceType: Double]]?

  @ObservedObject private var isShowingBalances = Preferences.Wallet.isShowingBalances

  init(
    image: AssetIconView,
    title: String,
    symbol: String,
    networkName: String,
    fiat: String,
    balance: String,
    shouldHideBalance: Bool = false,
    btcBalances: [String: [BTCBalanceType: Double]]? = nil
  ) {
    self.image = image
    self.title = title
    self.symbol = symbol
    self.networkName = networkName
    self.fiat = fiat
    self.balance = balance
    self.shouldHideBalance = shouldHideBalance
    self.btcBalances = btcBalances
  }

  var body: some View {
    AssetView(
      image: { image },
      title: title,
      symbol: symbol,
      networkName: networkName,
      accessoryContent: {
        Group {
          if shouldHideBalance && !isShowingBalances.value {
            Text("****")
          } else {
            VStack(alignment: .trailing) {
              Text(fiat.isEmpty ? "0.0" : fiat)
                .fontWeight(.semibold)
              Text(verbatim: "\(balance) \(symbol)")
            }
            .multilineTextAlignment(.trailing)
          }
        }
        .font(.footnote)
        .foregroundColor(Color(.braveLabel))
      }
    )
    .accessibilityLabel("\(title), \(balance) \(symbol), \(fiat)")
  }
}

#if DEBUG
struct FungibleAssetView_Previews: PreviewProvider {
  static var previews: some View {
    FungibleAssetView(
      image: AssetIconView(token: .previewToken, network: .mockMainnet),
      title: "Basic Attention Token",
      symbol: "BAT",
      networkName: "Ethereum Mainnet Beta",
      fiat: "$10,402.22",
      balance: "10303"
    )
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif

struct NFTAssetView: View {

  let image: NFTIconView
  let title: String
  let symbol: String
  let networkName: String
  let quantity: String

  var body: some View {
    AssetView(
      image: { image },
      title: title,
      symbol: symbol,
      networkName: networkName,
      accessoryContent: {
        Text(quantity)
          .font(.footnote)
          .foregroundColor(Color(.braveLabel))
      }
    )
    .accessibilityLabel("\(title), \(quantity) \(symbol)")
  }
}

#if DEBUG
struct NFTAssetView_Previews: PreviewProvider {
  static var previews: some View {
    NFTAssetView(
      image: NFTIconView(token: .previewToken, network: .mockMainnet),
      title: "Invisible Friends #3965",
      symbol: "INVSBLE",
      networkName: "Ethereum Mainnet Beta",
      quantity: "1"
    )
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif

/// AssetView is used to display an asset image, title, symbol and network with an optional accessory view on the right.
struct AssetView<ImageView: View, AccessoryContent: View>: View {
  let image: () -> ImageView
  let title: String
  let symbol: String
  let networkName: String
  @ViewBuilder var accessoryContent: () -> AccessoryContent

  var body: some View {
    HStack {
      image()
      VStack(alignment: .leading) {
        Text(title)
          .font(.footnote)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        Text(
          String.localizedStringWithFormat(
            Strings.Wallet.userAssetSymbolNetworkDesc,
            symbol,
            networkName
          )
        )
        .font(.caption)
        .foregroundColor(Color(.braveLabel))
      }
      .multilineTextAlignment(.leading)
      Spacer()
      accessoryContent()
    }
    .frame(maxWidth: .infinity)
    .padding(.vertical, 6)
    .accessibilityElement()
  }
}
