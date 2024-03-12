// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Shared
import StoreKit
import Combine
import Preferences
import os.log

/// The Store Environment for use with Debugging APIs
public enum BraveStoreEnvironment: String {
  /// The production environment
  case production
  
  /// The sandbox environment
  case sandbox
  
  /// Xcode environment - A debugger is attached
  case xcode
}

/// A structure representing a Product Group offered by Brave's Store
public enum BraveStoreProductGroup: String, CaseIterable {
  /// The group all VPN products belong to
  case vpn
  
  /// The group all Leo products belong to
  case leo
  
  /// The subscription group ID on App Store Connect
  public var groupID: String {
    #if DEBUG
    switch self {
    case .vpn: return "20625393"
    case .leo: return "21451390"
    }
    #else
    switch self {
    case .vpn: return "20621968"
    case .leo: return "21439231"
    }
    #endif
  }
  
  /// The SKU's associated environment domain
  public var skusDomain: String {
    #if DEBUG
    switch self {
    case .vpn: return "vpn.bravesoftware.com"
    case .leo: return "leo.bravesoftware.com"
    }
    #else
    switch self {
    case .vpn: return "vpn.brave.com"
    case .leo: return "leo.brave.com"
    }
    #endif
  }
}

/// A structure representing a Product offered by Brave's Store
public enum BraveStoreProduct: String, AppStoreProduct, CaseIterable {
  /// VPN Monthly AppStore SKU
  case vpnMonthly = "bravevpn.monthly"
  
  /// VPN Yearly AppStore SKU
  case vpnYearly = "bravevpn.yearly"
  
  /// Leo Monthly AppStore SKU
  case leoMonthly = "braveleo.monthly"
  
  /// Leo Yearly AppStore SKU
  case leoYearly = "braveleo.yearly"
  
  /// The Title of the SKU Group
  public var subscriptionGroup: String {
    switch self {
    case .vpnMonthly, .vpnYearly: return "Brave VPN"
    case .leoMonthly, .leoYearly: return "Brave Leo"
    }
  }
  
  /// The Brave Store's SKU
  public var itemSKU: String {
    // These are from the `Order.items.sku` and are NOT the same as the AppStore Skus
    // These are Brave's Skus
    switch self {
    case .vpnMonthly: return "brave-firewall-vpn-premium"
    case .vpnYearly: return "brave-firewall-vpn-premium-year"
    case .leoMonthly: return "brave-leo-premium"
    case .leoYearly: return "brave-leo-premium-year"
    }
  }
  
  public var group: BraveStoreProductGroup {
    switch self {
    case .vpnMonthly, .vpnYearly: return .vpn
    case .leoMonthly, .leoYearly: return .leo
    }
  }
}

/// A structure for handling Brave Store transactions, products, and purchases
public class BraveStoreSDK: AppStoreSDK {
  
  public static let shared = BraveStoreSDK()
  
  // MARK: - VPN
  
  /// The AppStore Vpn Monthly Product offering
  @Published
  private(set) var vpnMonthlyProduct: Product?
  
  /// The AppStore Vpn Yearly Product offering
  @Published
  private(set) var vpnYearlyProduct: Product?
  
  /// The AppStore Vpn customer purchase Subscription Status
  @Published
  private(set) var vpnSubscriptionStatus: Product.SubscriptionInfo.Status?
  
  // MARK: - LEO
  
  /// The AppStore Leo Monthly Product offering
  @Published
  private(set) var leoMonthlyProduct: Product?
  
  /// The AppStore Leo Monthly Product offering
  @Published
  private(set) var leoYearlyProduct: Product?
  
  /// The AppStore Leo customer purchase Subscription Status
  @Published
  private(set) var leoSubscriptionStatus: Product.SubscriptionInfo.Status?
  
  // MARK: - Private
  
  /// All observers this uses
  private var observers = [AnyCancellable]()
  
  private override init() {
    super.init()

    // Observe Product Updates
    observers.append($allProducts.sink(receiveValue: onProductsUpdated(_:)))
    
    // Observe Customer Purchase Updates
    observers.append($purchasedProducts.sink(receiveValue: onPurchasesUpdated(_:)))
  }
  
  // MARK: - Public
  
  /// A list of all products the Brave Store offers
  public override var allAppStoreProducts: [any AppStoreProduct] {
    return BraveStoreProduct.allCases
  }
  
  /// The current store environment
  public var environment: BraveStoreEnvironment {
    if AppConstants.buildChannel == .release {
      return .production
    }

    // Retrieve the subscription renewal information
    guard let renewalInfo = [vpnSubscriptionStatus, leoSubscriptionStatus].compactMap({ $0 }).first?.renewalInfo else {
      // There is currently no subscription so check if there was a restored receipt
      if Bundle.main.appStoreReceiptURL?.lastPathComponent == "sandboxReceipt" {
        return .sandbox
      }

      return .production
    }
    
    // Retrieve the current environment from StoreKit's Renewal Information
    switch renewalInfo {
    case .verified(let renewalInfo), .unverified(let renewalInfo, _):
      if #available(iOS 16.0, *) {
        return .init(rawValue: renewalInfo.environment.rawValue) ?? .production
      }
      
      return .init(rawValue: renewalInfo.environmentStringRepresentation) ?? .production
    }
  }
  
  /// A boolean indicating whether or not all the VPN product offerings has been loaded
  public var isVpnProductsLoaded: Bool {
    if vpnMonthlyProduct != nil || vpnYearlyProduct != nil {
      return true
    }
    
    return false
  }
  
  /// A boolean indicating whether or not all the Leo product offerings has been loaded
  public var isLeoProductsLoaded: Bool {
    if leoMonthlyProduct != nil || leoYearlyProduct != nil {
      return true
    }
    
    return false
  }
  
  /// Refreshes all the Skus SDK orders
  public func refreshAllSkusOrders() {
    
  }
  
  /// Restores a single purchased product
  /// - Parameter product: The product whose purchase receipt to restore
  /// - Returns: Returns true if receipt restoration was successful. False otherwise
  public func restorePurchase(_ product: BraveStoreProduct) async -> Bool {
    if await currentTransaction(for: product) != nil {
      try? await AppStoreReceipt.sync()
      return true
    }
    
    return false
  }
  
  /// Restores all purchased products
  /// - Returns: Returns true if receipt restoration was successful. False otherwise
  @MainActor
  public func restorePurchases() async -> Bool {
    #if STOREKIT2
    var didRestore = false
    for await result in Transaction.currentEntitlements {
      if case .verified(let transaction) = result {
        if let product = BraveStoreProduct(rawValue: transaction.productID) {
          // Update subscription status for the product
          switch product {
          case .vpnMonthly, .vpnYearly:
            vpnSubscriptionStatus = await transaction.subscriptionStatus
          case .leoMonthly, .leoYearly:
            leoSubscriptionStatus = await transaction.subscriptionStatus
          }
          
          // Update SkusSDK
          do {
            try await self.updateSkusPurchaseState(for: product)
            didRestore = true
          } catch {
            Logger.module.error("[BraveStoreSDK] - Failed to restore purchased product receipt: \(error)")
          }
        }
      }
    }
    return didRestore
    #else
    do {
      for product in BraveStoreProduct.allCases {
        try await self.updateSkusPurchaseState(for: product)
      }
      return true
    } catch {
      Logger.module.error("[BraveStoreSDK] - Failed to restore purchased product receipt: \(error)")
      return false
    }
    #endif
  }
  
  /// Purchases the specified product using In-App purchases
  /// - Parameter product: The product the customer wishes to purchase
  /// - Throws: An exception if purchasing fails for any reason.
  ///           Purchase may be successful with the AppStore, but fail with SkusSDK.
  @MainActor
  public func purchase(product: BraveStoreProduct) async throws {
    if let subscription = await subscription(for: product) {
      if let transaction = try await super.purchase(subscription) {
        
        // Update Skus SDK Purchase
        try await self.updateSkusPurchaseState(for: product)
      }
    }
  }
  
  // MARK: - Internal
  
  /// Observer function called when AppStore products have been fetched or updated
  private func onProductsUpdated(_ products: Products) {
    // Process only subscriptions at this time as Brave has no other products
    let products = products.all.filter({ $0.type == .autoRenewable })
    
    // No products to process
    if products.isEmpty {
      return
    }

    // Update vpn products
    vpnMonthlyProduct = products.first(where: { $0.id == BraveStoreProduct.vpnMonthly.rawValue })
    vpnYearlyProduct = products.first(where: { $0.id == BraveStoreProduct.vpnYearly.rawValue })
    
    // Update leo products
    leoMonthlyProduct = products.first(where: { $0.id == BraveStoreProduct.leoMonthly.rawValue })
    leoYearlyProduct = products.first(where: { $0.id == BraveStoreProduct.leoYearly.rawValue })
  }
  
  /// Observer function called when AppStore customer purchased products have been fetched or updated
  private func onPurchasesUpdated(_ products: Products) {
    // Process only subscriptions at this time as Brave has no other products
    let products = products.all.filter({ $0.type == .autoRenewable }).filter({ $0.subscription != nil })
    
    // No products to process
    if products.isEmpty {
      return
    }
    
    Task { @MainActor [weak self] in
      guard let self = self else { return }
      
      // Retrieve subscription statuses
      let vpnSubscriptions = products.filter({ $0.id == BraveStoreProduct.vpnMonthly.rawValue || $0.id == BraveStoreProduct.vpnYearly.rawValue }).compactMap({ $0.subscription })
      let leoSubscriptions = products.filter({ $0.id == BraveStoreProduct.leoMonthly.rawValue || $0.id == BraveStoreProduct.leoYearly.rawValue }).compactMap({ $0.subscription })
      
      // Statuses apply to the entire group
      vpnSubscriptionStatus = try? await vpnSubscriptions.first?.status.first
      leoSubscriptionStatus = try? await leoSubscriptions.first?.status.first
    }
  }
  
  /// Refreshes a Skus-SDK product order
  /// - Throws: An exception if refreshing the order information fails
  @MainActor
  private func refreshOrder(for productGroup: BraveStoreProductGroup) async throws {
    // This SDK currently only supports Leo
    // until we update the VPN code to use it
    if productGroup == .vpn {
      return
    }
    
    // Attempt to update the Application Bundle's receipt, if necessary
    if (try? AppStoreReceipt.receipt) == nil {
      try await AppStoreReceipt.sync()
    }
    
    // Create a Skus-SDK for the specified product
    let skusSDK = BraveSkusSDK()
    
    // Create an order for the AppStore receipt
    // If an order already exists, refreshes the order information
    if let orderId = Preferences.AIChat.subscriptionOrderId.value {
      try await skusSDK.refreshOrder(orderId: orderId, for: productGroup)
      return
    }
    
    throw BraveSkusSDK.SkusError.cannotCreateOrder
  }
  
  /// Updates the Skus-SDK credentials and order information
  /// - Parameter product: The product whose information to update
  /// - Throws: An exception if updating the purchase information fails
  @MainActor
  private func updateSkusPurchaseState(for product: BraveStoreProduct) async throws {
    // This SDK currently only supports Leo
    // until we update the VPN code to use it
    if product.group == .vpn {
      return
    }
    
    // Attempt to update the Application Bundle's receipt, if necessary
    if (try? AppStoreReceipt.receipt) == nil {
      try await AppStoreReceipt.sync()
    }
    
    // Create a Skus-SDK for the specified product
    let skusSDK = BraveSkusSDK()
    
    // Create an order for the AppStore receipt
    // If an order already exists, refreshes the order information
    var orderId = Preferences.AIChat.subscriptionOrderId.value
    if orderId == nil {
      orderId = try await skusSDK.createOrder(for: product)
      Preferences.AIChat.subscriptionOrderId.value = orderId
    }
    
    guard let orderId = orderId else {
      throw BraveSkusSDK.SkusError.cannotCreateOrder
    }

    // There's an existing with no expiry date, refresh it
    var expiryDate = Preferences.AIChat.subscriptionExpirationDate.value
    if expiryDate == nil {
      let order = try await skusSDK.refreshOrder(orderId: orderId, for: product.group)
      expiryDate = order.expiresAt
      Preferences.AIChat.subscriptionExpirationDate.value = expiryDate
      Preferences.AIChat.subscriptionHasCredentials.value = true  // Refreshing an order updates credentials
    }
    
    guard let expiryDate = expiryDate else {
      throw BraveSkusSDK.SkusError.cannotCreateOrder
    }
    
    // If the order is expired, refresh it
    if Date() > expiryDate {
      let order = try await skusSDK.refreshOrder(orderId: orderId, for: product.group)
      Preferences.AIChat.subscriptionExpirationDate.value = order.expiresAt
      Preferences.AIChat.subscriptionHasCredentials.value = true  // Refreshing an order updates credentials
      return
    }
    
    // There is an order, and an expiry date, but no credentials
    // Fetch the credentials
    if !Preferences.AIChat.subscriptionHasCredentials.value {
      try await skusSDK.fetchCredentials(orderId: orderId, for: product.group)
      Preferences.AIChat.subscriptionHasCredentials.value = true
    }
  }
}
