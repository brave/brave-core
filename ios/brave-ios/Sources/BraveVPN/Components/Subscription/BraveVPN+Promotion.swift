// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import GuardianConnect
import Preferences
import os.log

extension BraveVPN {

  /// Editing product promotion order first yearly and monthly after
  @MainActor public static func updateStorePromotionOrder() async {
    let storePromotionController = SKProductStorePromotionController.default()
    // Fetch Products
    guard let yearlyProduct = BraveVPNProductInfo.yearlySubProduct,
      let monthlyProduct = BraveVPNProductInfo.monthlySubProduct
    else {
      Logger.module.debug("Found empty while fetching SKProducts for promotion order")
      return
    }

    // Update the order
    do {
      try await storePromotionController.update(promotionOrder: [yearlyProduct, monthlyProduct])
    } catch {
      Logger.module.debug("Error while opdating product promotion order ")
    }
  }

  /// Hiding Store pormotion if the active subscription for the type
  @MainActor public static func hideActiveStorePromotion() async {
    let storePromotionController = SKProductStorePromotionController.default()

    // Fetch Products
    guard let yearlyProduct = BraveVPNProductInfo.yearlySubProduct,
      let monthlyProduct = BraveVPNProductInfo.monthlySubProduct
    else {
      Logger.module.debug("Found empty while fetching SKProducts for promotion order")
      return
    }

    // No promotion for VPN is purchased through website side
    if Preferences.VPN.skusCredential.value != nil {
      await hideSubscriptionType(yearlyProduct)
      await hideSubscriptionType(monthlyProduct)

      return
    }

    // Hide the promotion
    let activeSubscriptionType = BraveVPN.activeSubscriptionType

    switch activeSubscriptionType {
    case .monthly:
      await hideSubscriptionType(monthlyProduct)
    case .yearly:
      await hideSubscriptionType(yearlyProduct)
    default:
      break
    }

    func hideSubscriptionType(_ product: SKProduct) async {
      do {
        try await storePromotionController.update(promotionVisibility: .hide, for: product)
      } catch {
        Logger.module.debug("Error while opdating product promotion order ")
      }
    }
  }

  public static func activatePaymentTypeForStoredPromotion(savedPayment: SKPayment?) {
    if let payment = savedPayment {
      SKPaymentQueue.default().add(payment)
    }

    iapObserver.savedPayment = nil
  }
}
