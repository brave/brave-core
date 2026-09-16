// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStore
import Preferences
import StoreKit
import os.log

extension BraveVPN {

  /// Editing product promotion order first yearly and monthly after
  @MainActor public static func updateStorePromotionOrder() async {
    do {
      try await Product.PromotionInfo.updateProductOrder(byID: [
        BraveStoreProduct.vpnYearly.rawValue,
        BraveStoreProduct.vpnMonthly.rawValue,
      ])
    } catch {
      Logger.module.debug("Error while updating product promotion order")
    }
  }

  /// Hiding Store pormotion if the active subscription for the type
  @MainActor public static func hideActiveStorePromotion() async {
    // No promotion for VPN is purchased through website side
    if Preferences.VPN.skusCredential.value != nil {
      await hideSubscriptionType(BraveStoreProduct.vpnYearly.rawValue)
      await hideSubscriptionType(BraveStoreProduct.vpnMonthly.rawValue)

      return
    }

    // Hide the promotion
    let activeSubscriptionType = BraveVPN.activeSubscriptionType

    switch activeSubscriptionType {
    case .monthly:
      await hideSubscriptionType(BraveStoreProduct.vpnMonthly.rawValue)
    case .yearly:
      await hideSubscriptionType(BraveStoreProduct.vpnYearly.rawValue)
    default:
      break
    }
  }

  private static func hideSubscriptionType(_ productID: String) async {
    do {
      try await Product.PromotionInfo.updateProductVisibility(.hidden, for: productID)
    } catch {
      Logger.module.debug("Error while updating product promotion visibility")
    }
  }

  @MainActor public static func activatePaymentTypeForStoredPromotion(product: Product?) {
    if let product {
      Task {
        await iapObserver.purchase(product: product)
      }
    }

    iapObserver.savedPromotedProduct = nil
  }
}
