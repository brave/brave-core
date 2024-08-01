// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStore
import DesignSystem
import StoreKit
import SwiftUI

enum BraveVPNSubscriptionTier {
  case monthly
  case yearly
}

struct BraveVPNPremiumTierSelectionView: View {
  var originalProduct: SKProduct?
  var discountedProduct: SKProduct?

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
            Text(type == .yearly ? "One Year" : "Monthly Subscription")
              .font(.body.weight(.semibold))
              .foregroundStyle(.white)

            if type == .yearly {
              Text("BEST VALUE")
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

            if let product = originalProduct,
              let formattedPrice = product.price.frontSymbolCurrencyFormatted(
                with: product.priceLocale
              )
            {
              createPriceTagLabel(product: product, price: formattedPrice)
            } else {
              ProgressView()
                .tint(Color.white)
            }
          }

          HStack(spacing: 2) {
            Text(type == .yearly ? "Renews Annually" : "Renews Monthly")
              .font(.subheadline)
              .foregroundColor(Color(braveSystemName: .primitiveBlurple95))

            if type == .yearly {
              createDiscountPercentageLabel()
            }

            Spacer()

            if type == .yearly, let product = originalProduct {
              createDiscountTagLabel(product: product, price: "")
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
  private func createPriceTagLabel(product: SKProduct, price: String) -> some View {
    Text(
      "\(product.priceLocale.currency?.identifier ?? "")"
    )
    .font(.footnote)
    .foregroundColor(Color(braveSystemName: .primitiveBlurple95))

      + Text(
        " \(price)"
      )
      .font(.headline)
      .foregroundColor(.white)

      + Text(
        "/ \(type == .monthly ? "month" : "year")"
      )
      .font(.footnote)
      .foregroundColor(Color(braveSystemName: .primitiveBlurple95))
  }

  @ViewBuilder
  private func createDiscountPercentageLabel() -> some View {
    let yearlyDouble = originalProduct?.price.doubleValue ?? 0.0
    let discountDouble = discountedProduct?.price.multiplying(by: 12).doubleValue ?? 0.0
    let discountSavingPercentage =
      discountDouble > 0.0 ? 100 - Int((yearlyDouble * 100) / discountDouble) : 0

    Text("(")
      .font(.subheadline)
      .foregroundColor(Color(braveSystemName: .primitiveBlurple95))
      + Text("save \(discountSavingPercentage)%")
      .underline()
      .font(.subheadline)
      .foregroundColor(Color(braveSystemName: .primitiveBlurple95))
      + Text(")")
      .font(.subheadline)
      .foregroundColor(Color(braveSystemName: .primitiveBlurple95))
  }

  @ViewBuilder
  private func createDiscountTagLabel(product: SKProduct, price: String) -> some View {
    let discountDouble =
      discountedProduct?.price.multiplying(by: 12).frontSymbolCurrencyFormatted(
        with: product.priceLocale
      ) ?? ""

    Group {
      Text(
        "\(product.priceLocale.currency?.identifier ?? "") "
      )
      .font(.footnote)
      .foregroundColor(Color(braveSystemName: .primitiveBlurple95))
      .strikethrough()

        + Text("\(discountDouble)")
        .font(.subheadline.weight(.semibold))
        .kerning(0.075)
        .strikethrough()
        .foregroundColor(.white)
    }
    .opacity(0.6)
  }
}

#if DEBUG
struct BraveVPNPremiumTierSelectionView_Previews: PreviewProvider {
  static var previews: some View {
    let mockMonthlyProduct = BraveVPNMockSKProduct(
      price: NSDecimalNumber(string: "9.99"),
      priceLocale: Locale(identifier: "en_US")
    )

    let mockYearlyProduct = BraveVPNMockSKProduct(
      price: NSDecimalNumber(string: "99.99"),
      priceLocale: Locale(identifier: "en_US")
    )

    VStack {
      BraveVPNPremiumTierSelectionView(
        originalProduct: mockYearlyProduct,
        discountedProduct: mockMonthlyProduct,
        type: .yearly,
        selectedTierType: Binding.constant(.yearly)
      )

      BraveVPNPremiumTierSelectionView(
        originalProduct: mockYearlyProduct,
        discountedProduct: mockMonthlyProduct,
        type: .yearly,
        selectedTierType: Binding.constant(.monthly)
      )

      BraveVPNPremiumTierSelectionView(
        originalProduct: mockMonthlyProduct,
        discountedProduct: nil,
        type: .monthly,
        selectedTierType: Binding.constant(.monthly)
      )

      BraveVPNPremiumTierSelectionView(
        originalProduct: mockMonthlyProduct,
        discountedProduct: nil,
        type: .monthly,
        selectedTierType: Binding.constant(.yearly)
      )
    }
    .padding()
    .background(
      Color(braveSystemName: .primitivePrimary10)
    )
  }
}
#endif
