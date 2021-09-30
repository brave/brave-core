/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI

struct PortfolioAssetView: View {
  var image: UIImage
  var title: String
  var symbol: String
  var amount: String
  var quantity: String
  
  var body: some View {
    HStack {
      Circle()
        .frame(width: 40, height: 40)
        .overlay(
          Image(uiImage: image)
        )
        .clipShape(Circle())
      VStack(alignment: .leading) {
        Text(title)
          .font(.footnote)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        Text(symbol)
          .font(.caption)
          .foregroundColor(Color(.braveLabel))
      }
      Spacer()
      VStack(alignment: .trailing) {
        Text(amount.isEmpty ? "0.0" : amount)
        Text("\(quantity) \(symbol)")
      }
      .font(.footnote)
      .foregroundColor(Color(.braveLabel))
    }
    .frame(maxWidth: .infinity)
    .padding(.vertical, 6)
  }
}

#if DEBUG
struct PortfolioAssetView_Previews: PreviewProvider {
  static var previews: some View {
    PortfolioAssetView(
      image: .init(),
      title: "Basic Attention Token",
      symbol: "BAT",
      amount: "$10,402.22",
      quantity: "10303"
    )
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
