/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI

struct PortfolioAssetView: View {
  var image: AssetIconView
  var title: String
  var symbol: String
  let networkName: String
  var amount: String
  var quantity: String

  var body: some View {
    AssetView(
      image: { image },
      title: title,
      symbol: symbol,
      networkName: networkName,
      accessoryContent: {
        VStack(alignment: .trailing) {
          Text(amount.isEmpty ? "0.0" : amount)
            .fontWeight(.semibold)
          Text(verbatim: "\(quantity) \(symbol)")
        }
        .font(.footnote)
        .foregroundColor(Color(.braveLabel))
        .multilineTextAlignment(.trailing)
      }
    )
    .accessibilityLabel("\(title), \(quantity) \(symbol), \(amount)")
  }
}

#if DEBUG
struct PortfolioAssetView_Previews: PreviewProvider {
  static var previews: some View {
    PortfolioAssetView(
      image: AssetIconView(token: .previewToken, network: .mockMainnet),
      title: "Basic Attention Token",
      symbol: "BAT",
      networkName: "Ethereum Mainnet Beta",
      amount: "$10,402.22",
      quantity: "10303"
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
  let accessoryContent: () -> AccessoryContent

  var body: some View {
    HStack {
      image()
      VStack(alignment: .leading) {
        Text(title)
          .font(.footnote)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        Text(String.localizedStringWithFormat(Strings.Wallet.userAssetSymbolNetworkDesc, symbol, networkName))
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
