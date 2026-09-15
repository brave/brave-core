// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStore
import DesignSystem
import StoreKit
import SwiftUI

enum BraveVPNSubscriptionTier: String {
  case monthly = "yearly"
  case yearly = "monthly"
}

struct BraveVPNPremiumTierSelectionView: View {
  var originalProduct: Product?
  var discountedProduct: Product?

  var type: BraveVPNSubscriptionTier

  @Binding
  var selectedTierType: BraveVPNSubscriptionTier

  var body: some View {
    Button(
      action: {
        selectedTierType = type
      },
      label: {
        VStack(alignment: .leading, spacing: 8.0) {
          HStack {
            Text(type == .yearly ? Strings.VPN.yearlySubTitle : Strings.VPN.monthlySubTitle)
              .font(.headline)
              .foregroundStyle(.white)

            if type == .yearly {
              Text(Strings.VPN.yearlySubDisclaimer)
                .font(.caption2.weight(.bold))
                .foregroundColor(.white)
                .padding(4.0)
                .background(
                  LinearGradient(
                    gradient:
                      Gradient(colors: [
                        Color(UIColor(rgb: 0xFF4000)),
                        Color(UIColor(rgb: 0xFF1F01)),
                      ]),
                    startPoint: .init(x: 0.26, y: 0.0),
                    endPoint: .init(x: 0.26, y: 1.0)
                  )
                )
                .clipShape(RoundedRectangle(cornerRadius: 4.0, style: .continuous))
            }

            Spacer()

            if let product = originalProduct {
              createPriceTagLabel(product: product)
            } else {
              ProgressView()
                .tint(Color.white)
            }
          }

          HStack(spacing: 2) {
            Text(type == .yearly ? Strings.VPN.renewAnnually : Strings.VPN.renewMonthly)
              .font(.subheadline)
              .foregroundColor(Color(braveSystemName: .primitiveBlurple95))

            if type == .yearly {
              createDiscountPercentageLabel()
            }

            Spacer()

            if type == .yearly, let product = originalProduct {
              createDiscountTagLabel(product: product)
            }
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

  @ViewBuilder
  private func createPriceTagLabel(product: Product) -> some View {
    let locale = product.priceFormatStyle.locale
    let currencyIdentifier = locale.currency?.identifier ?? ""
    let currencySymbol = locale.currencySymbol ?? ""

    Text("\(currencyIdentifier)\(currencySymbol)")
      .font(.footnote)
      .foregroundColor(Color(braveSystemName: .primitiveBlurple95))

      + Text(product.price, format: .currency(code: "").locale(locale))
      .font(.headline)
      .foregroundColor(.white)

      + Text(
        "/ \(type == .monthly ? Strings.VPN.paywallMonthlyPriceDividend : Strings.VPN.paywallYearlyPriceDividend)"
      )
      .font(.footnote)
      .foregroundColor(Color(braveSystemName: .primitiveBlurple95))
  }

  @ViewBuilder
  private func createDiscountPercentageLabel() -> some View {
    let yearlyDouble = originalProduct.map { NSDecimalNumber(decimal: $0.price).doubleValue } ?? 0.0
    let discountDouble =
      discountedProduct.map { NSDecimalNumber(decimal: $0.price * 12).doubleValue } ?? 0.0
    let discountSavingPercentage =
      discountDouble > 0.0 ? 100 - Int((yearlyDouble * 100) / discountDouble) : 0

    Group {
      Text("(")
        + Text("\(Strings.VPN.save) \(discountSavingPercentage)%")
        .underline()
        .fontWeight(.semibold)
        + Text(")")
    }
    .foregroundColor(Color(braveSystemName: .primitiveBlurple95))
    .font(.subheadline)
  }

  @ViewBuilder
  private func createDiscountTagLabel(product: Product) -> some View {
    let locale = product.priceFormatStyle.locale
    let currencyIdentifier = locale.currency?.identifier ?? ""
    let currencySymbol = locale.currencySymbol ?? ""

    Group {
      if let discountedProduct {
        Text("\(currencyIdentifier)\(currencySymbol)")
          .font(.footnote)
          .foregroundColor(Color(braveSystemName: .primitiveBlurple95))
          + Text(discountedProduct.price * 12, format: .currency(code: "").locale(locale))
          .font(.subheadline.weight(.semibold))
          .kerning(0.075)
          .foregroundColor(.white)
      }
    }
    .strikethrough()
    .opacity(0.6)
  }
}

#if DEBUG
#Preview {
  VStack {
    BraveVPNPremiumTierSelectionView(
      originalProduct: nil,
      discountedProduct: nil,
      type: .yearly,
      selectedTierType: Binding.constant(.yearly)
    )

    BraveVPNPremiumTierSelectionView(
      originalProduct: nil,
      discountedProduct: nil,
      type: .monthly,
      selectedTierType: Binding.constant(.monthly)
    )
  }
  .padding()
  .background(
    Color(braveSystemName: .primitivePrimary10)
  )
}
#endif
