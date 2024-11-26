// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStore
import Foundation
import Preferences
import StoreKit
import SwiftUI
import os.log

public class AIChatSubscriptionDetailModelView: ObservableObject {
  enum SkuOrderStatus {
    case loading
    case inactive
    case expired
    case active
  }

  enum SkuOrderType {
    case notDetermined
    case monthly
    case yearly
  }

  @ObservedObject
  private var storeSDK = BraveStoreSDK.shared

  @Published
  private(set) var credentialSummary: SkusCredentialSummary?

  private var isLoading = false

  var skuOrderStatus: SkuOrderStatus {
    if isLoading {
      return .loading
    }

    guard let order = credentialSummary?.order else {
      return .inactive
    }

    if let expiresAt = order.expiresAt {
      return Date() > expiresAt ? .expired : .active
    }

    return .inactive
  }

  public init() {

  }

  @MainActor
  func fetchCredentialSummary() async {
    self.isLoading = true

    if storeSDK.leoSubscriptionStatus != nil {
      self.isLoading = false
      return
    }

    do {
      let credentialSummary = try await BraveSkusSDK.shared.credentialsSummary(for: .leo)
      self.credentialSummary = credentialSummary
    } catch {
      Logger.module.error("Error Fetching Skus Credential Summary: \(error)")
    }

    self.isLoading = false
  }

  var skuOrderExpirationDate: Date? {
    guard let order = credentialSummary?.order, let expiresAt = order.expiresAt else {
      return nil
    }

    return expiresAt
  }

  var skuOrderProductType: SkuOrderType? {
    // SkusSDK only returns `brave-leo-premium` as the sku for the Order Items
    if skuOrderStatus == .active {
      guard let itemSKU = credentialSummary?.order.items.first?.sku else {
        return .notDetermined
      }

      if itemSKU == BraveStoreProduct.leoMonthly.itemSKU {
        return .monthly
      }

      if itemSKU == BraveStoreProduct.leoYearly.itemSKU {
        return .yearly
      }

      return .notDetermined
    }

    return nil
  }

  var inAppPurchasedProductType: BraveStoreProduct? {
    if let status = storeSDK.leoSubscriptionStatus {
      if case .verified(let transaction) = status.transaction {
        if transaction.productID == BraveStoreProduct.leoMonthly.rawValue {
          return .leoMonthly
        }

        if transaction.productID == BraveStoreProduct.leoYearly.rawValue {
          return .leoYearly
        }
      }
    }

    return nil
  }

  var inAppPurchaseSubscriptionState: Product.SubscriptionInfo.RenewalState? {
    storeSDK.leoSubscriptionStatus?.state
  }

  var inAppPurchaseSubscriptionPeriod: StoreKit.Product.SubscriptionPeriod? {
    if let status = storeSDK.leoSubscriptionStatus {
      if case .verified(let transaction) = status.transaction {
        if transaction.productID == BraveStoreProduct.leoMonthly.rawValue {
          return storeSDK.leoMonthlyProduct?.subscription?.subscriptionPeriod
        }

        if transaction.productID == BraveStoreProduct.leoYearly.rawValue {
          return storeSDK.leoYearlyProduct?.subscription?.subscriptionPeriod
        }
      }
    }

    return nil
  }

  var inAppPurchaseSubscriptionExpiryDate: Date? {
    if let status = storeSDK.leoSubscriptionStatus {
      if case .verified(let transaction) = status.transaction {
        return transaction.expirationDate
      }
    }

    return nil
  }

  var inAppPurchaseProductsLoaded: Bool {
    storeSDK.isLeoProductsLoaded
  }

  var canDisplaySubscriptionStatus: Bool {
    return storeSDK.leoSubscriptionStatus?.state == .subscribed
      || skuOrderStatus == .active
      || skuOrderStatus == .loading
  }

  var canSubscriptionBeLinked: Bool {
    // Check subscription is activated with in-app purchase
    if storeSDK.leoSubscriptionStatus?.state != nil {
      // Order status is active no need to link purchase
      if skuOrderStatus != .loading, skuOrderStatus != .active {
        return true
      }
    }

    return false
  }

  var isSubscriptionStatusLoaded: Bool {
    return storeSDK.leoSubscriptionStatus?.state != nil || credentialSummary != nil
  }

  var isDevReceiptLinkingAvailable: Bool {
    storeSDK.environment != .production
  }
}
