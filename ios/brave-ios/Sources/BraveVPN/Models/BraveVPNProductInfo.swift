// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStore
import Foundation
import Shared
import StoreKit
import os.log

public class BraveVPNProductInfo {
  // Prices are fetched once per launch and kept in memory.
  // If the prices could not be fetched, we retry after user tries to go to buy-vpn screen.
  static var monthlySubProduct: Product?
  static var yearlySubProduct: Product?

  /// Whether we have enough product info to present to the user.
  /// If the user has bought the vpn already, it returns `true` since we do not need price details anymore.
  public static var isComplete: Bool {
    switch BraveVPN.vpnState {
    case .purchased:
      return true
    case .notPurchased, .expired:
      guard let monthlyPlan = monthlySubProduct, let yearlyPlan = yearlySubProduct else {
        return false
      }

      // Make sure the price can be displayed correctly.
      return !monthlyPlan.displayPrice.isEmpty && !yearlyPlan.displayPrice.isEmpty
    }
  }

  public init() {}

  public func load() {
    Task {
      do {
        let products = try await Product.products(for: [
          BraveStoreProduct.vpnMonthly.rawValue,
          BraveStoreProduct.vpnYearly.rawValue,
        ])

        for product in products {
          switch product.id {
          case BraveStoreProduct.vpnMonthly.rawValue:
            BraveVPNProductInfo.monthlySubProduct = product
          case BraveStoreProduct.vpnYearly.rawValue:
            BraveVPNProductInfo.yearlySubProduct = product
          default:
            assertionFailure("Found product identifier that doesn't match")
          }
        }
      } catch {
        Logger.module.error("Failed to fetch VPN AppStore products: \(error.localizedDescription)")
      }
    }
  }
}
