// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import StoreKit
import SwiftUI

enum BraveVPNSubscriptionTier {
  case monthly
  case yearly
}

struct BraveVPNPremiumTierSelectionView: View {
  var title: String
  var description: String?
  var product: Product?
  var type: BraveVPNSubscriptionTier

  @Binding
  var selectedTierType: BraveVPNSubscriptionTier

  var body: some View {
    Button(
      action: {
        selectedTierType = type
      },
      label: {
        HStack {
          VStack(alignment: .leading, spacing: 8.0) {
            Text(title)
              .font(.title2.weight(.semibold))
              .foregroundColor(Color(.white))

            if let description = description {
              Text(description)
                .font(.caption2.weight(.semibold))
                .foregroundColor(Color(braveSystemName: .green50))
                .padding(4.0)
                .background(Color(braveSystemName: .green20))
                .clipShape(RoundedRectangle(cornerRadius: 4.0, style: .continuous))
            }
          }
          Spacer()

          if let product = product {
            HStack(alignment: .center, spacing: 2) {
              Text(
                "\(product.priceFormatStyle.locale.currency?.identifier ?? "")\(product.priceFormatStyle.locale.currencySymbol ?? "")"
              )
              .font(.subheadline)
              .foregroundColor(Color(braveSystemName: .primitivePrimary80))

              Text(
                product.price.frontSymbolCurrencyFormatted(
                  with: product.priceFormatStyle.locale,
                  isSymbolIncluded: false
                ) ?? product.displayPrice
              )
              .font(.title)
              .foregroundColor(.white)

              Text(
                " / \(type == .monthly ? "month" : "year")"
              )
              .font(.subheadline)
              .foregroundColor(Color(braveSystemName: .primitivePrimary80))
            }
          } else {
            ProgressView()
              .tint(Color.white)
          }
        }
        .padding()
        .background(
          Color(
            braveSystemName: selectedTierType == type ? .primitivePrimary40 : .primitivePrimary20
          )
        )
        .overlay(
          ContainerRelativeShape()
            .strokeBorder(
              Color(braveSystemName: .primitivePrimary50),
              lineWidth: selectedTierType == type ? 2.0 : 0.0
            )
        )
        .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
      }
    )
    .frame(maxWidth: .infinity)
    .buttonStyle(.plain)
  }
}

#if DEBUG
#Preview("VPNSelectionTierView") {
  VStack {
    BraveVPNPremiumTierSelectionView(
      title: "Monthly",
      description: nil,
      product: nil,
      type: .yearly,
      selectedTierType: Binding.constant(.yearly)
    )
    
    BraveVPNPremiumTierSelectionView(
      title: "Yearly",
      description: "%17 Discount",
      product: nil,
      type: .monthly,
      selectedTierType: Binding.constant(.yearly)
    )
  }
  .padding()
  .background(
    Color(braveSystemName: .primitivePrimary10)
  )
}
#endif
