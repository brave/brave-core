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
    HStack {
      image
      VStack(alignment: .leading) {
        Text(title)
          .font(.footnote)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        Text(String.localizedStringWithFormat(Strings.Wallet.userAssetSymbolNetworkDesc, symbol, networkName))
          .font(.caption)
          .foregroundColor(Color(.braveLabel))
      }
      Spacer()
      VStack(alignment: .trailing) {
        Text(amount.isEmpty ? "0.0" : amount)
        Text(verbatim: "\(quantity) \(symbol)")
      }
      .font(.footnote)
      .foregroundColor(Color(.braveLabel))
    }
    .frame(maxWidth: .infinity)
    .padding(.vertical, 6)
    .accessibilityElement()
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

struct PortfolioNFTAssetView: View {
  
  let image: NFTIconView
  let title: String
  let symbol: String
  let networkName: String
  let quantity: String
  
  var body: some View {
    HStack {
      image
      VStack(alignment: .leading) {
        Text(title)
          .font(.footnote)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        Text(String.localizedStringWithFormat(Strings.Wallet.userAssetSymbolNetworkDesc, symbol, networkName))
          .font(.caption)
          .foregroundColor(Color(.braveLabel))
      }
      Spacer()
      Text(quantity)
        .font(.footnote)
        .foregroundColor(Color(.braveLabel))
    }
    .frame(maxWidth: .infinity)
    .padding(.vertical, 6)
    .accessibilityElement()
    .accessibilityLabel("\(title), \(quantity) \(symbol)")
  }
}

#if DEBUG
struct PortfolioNFTAssetView_Previews: PreviewProvider {
  static var previews: some View {
    PortfolioNFTAssetView(
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
