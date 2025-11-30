// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Combine
import Foundation
import Preferences
import Shared
import StoreKit
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

  /// The group all Origin products belong to
  case origin

  /// The subscription group ID on App Store Connect
  public var groupID: String {
    switch BraveSkusEnvironment.current {
    case .beta:
      switch self {
      case .vpn: return "21497512"
      case .leo: return "21497453"
      case .origin: return "21734932"
      }
    case .nightly:
      switch self {
      case .vpn: return "21497623"
      case .leo: return "21497565"
      case .origin: return "21734930"
      }
    case .release:
      switch self {
      case .vpn: return "20621968"
      case .leo: return "21439231"
      case .origin: return "21734868"
      }
    }
  }

  /// The SKU's associated environment domain
  public var skusDomain: String {
    switch self {
    case .vpn: return BraveDomains.serviceDomain(prefix: "vpn")
    case .leo:
      // AI Chat uses staging as its default in unofficial builds. The passed in environment is only
      // used in unofficial builds and when no service override switch is in place.
      return BraveDomains.serviceDomain(prefix: "leo", environment: .staging)
    case .origin: return BraveDomains.serviceDomain(prefix: "origin")
    }
  }
}

/// A structure representing a Product offered by Brave's Store
public enum BraveStoreProduct: String, AppStoreProduct, CaseIterable {
  /// VPN Monthly AppStore SKU
  case vpnMonthly

  /// VPN Yearly AppStore SKU
  case vpnYearly

  /// Leo Monthly AppStore SKU
  case leoMonthly

  /// Leo Yearly AppStore SKU
  case leoYearly

  /// Origin Monthly AppStore SKU
  case originMonthly

  /// Origin Yearly AppStore SKU
  case originYearly

  public init?(rawValue: String) {
    if let value = Self.allCases.first(where: { $0.rawValue == rawValue }) {
      self = value
      return
    }

    if let value = Self.allCases.first(where: { $0.itemSKU == rawValue }) {
      self = value
      return
    }

    return nil
  }

  /// The Title of the SKU Group
  public var subscriptionGroup: String {
    switch self {
    case .vpnMonthly, .vpnYearly: return "Brave VPN"
    case .leoMonthly, .leoYearly: return "Brave Leo"
    case .originMonthly, .originYearly: return "Brave Origin"
    }
  }

  /// The Brave Store's SKU
  public var itemSKU: String {
    // These are from the `Order.items.sku` and are NOT the same as the AppStore Skus
    // These are Brave's Skus
    switch self {
    case .vpnMonthly: return "brave-vpn-premium"
    case .vpnYearly: return "brave-vpn-premium-year"
    case .leoMonthly: return "brave-leo-premium"
    case .leoYearly: return "brave-leo-premium-year"
    case .originMonthly: return "brave-origin-premium"
    case .originYearly: return "brave-origin-premium-year"
    }
  }

  /// The AppStore Product Subscription Group
  public var group: BraveStoreProductGroup {
    switch self {
    case .vpnMonthly, .vpnYearly: return .vpn
    case .leoMonthly, .leoYearly: return .leo
    case .originMonthly, .originYearly: return .origin
    }
  }

  /// The AppStore Product Subscription ID
  /// For release, there is no prefix.
  /// For nightly & beta, there is a prefix
  ///  - [beta | nightly].bravevpn.monthly
  ///  - [beta | nightly]].bravevpn.yearly
  ///  - [beta | nightly].braveleo.monthly
  ///  - [beta | nightly]].braveleo.yearly
  public var rawValue: String {
    var prefix: String {
      switch BraveSkusEnvironment.current {
      case .beta: return "beta."
      case .nightly: return "nightly."
      case .release: return ""
      }
    }

    var productId: String {
      switch self {
      case .vpnMonthly, .vpnYearly: return "bravevpn"
      case .leoMonthly, .leoYearly: return "braveleo"
      case .originMonthly, .originYearly: return "braveorigin"
      }
    }

    switch self {
    case .vpnMonthly, .leoMonthly, .originMonthly: return "\(prefix)\(productId).monthly"
    case .vpnYearly, .originYearly: return "\(prefix)\(productId).yearly"
    case .leoYearly:
      if BraveSkusEnvironment.current == .release {
        return "\(prefix)\(productId).yearly.2"
      }
      return "\(prefix)\(productId).yearly"
    }
  }
}

/// A structure for handling Brave Store transactions, products, and purchases
public class BraveStoreSDK: AppStoreSDK {

  @available(iOS, deprecated, message: "Create a new BraveStoreSDK instance instead.")
  public static let shared = BraveStoreSDK(
    skusService: Skus.SkusServiceFactory.get(privateMode: false)
  )

  // MARK: - Error

  /// A BraveStoreSDK Error
  enum BraveStoreSDKError: Error {
    /// The product doesn't exist or there is a mismatch between the AppStore product and Brave's offered products
    case invalidProduct
  }

  // MARK: - VPN

  /// The AppStore Vpn Monthly Product offering
  @Published
  private(set) public var vpnMonthlyProduct: Product?

  /// The AppStore Vpn Yearly Product offering
  @Published
  private(set) public var vpnYearlyProduct: Product?

  /// The AppStore Vpn customer purchase Subscription Status
  @Published
  private(set) public var vpnSubscriptionStatus: Product.SubscriptionInfo.Status?

  // MARK: - LEO

  /// The AppStore Leo Monthly Product offering
  @Published
  private(set) public var leoMonthlyProduct: Product?

  /// The AppStore Leo Monthly Product offering
  @Published
  private(set) public var leoYearlyProduct: Product?

  /// The AppStore Leo customer purchase Subscription Status
  @Published
  private(set) public var leoSubscriptionStatus: Product.SubscriptionInfo.Status?

  // MARK: - ORIGIN

  /// The AppStore Origin Monthly Product offering
  @Published
  private(set) public var originMonthlyProduct: Product?

  /// The AppStore Origin Monthly Product offering
  @Published
  private(set) public var originYearlyProduct: Product?

  /// The AppStore Origin customer purchase Subscription Status
  @Published
  private(set) public var originSubscriptionStatus: Product.SubscriptionInfo.Status?

  // MARK: - Private

  /// All observers this uses
  private var observers = [AnyCancellable]()

  private let skusService: (any SkusSkusService)?

  public init(skusService: (any SkusSkusService)?) {
    self.skusService = skusService

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
    guard
      let renewalInfo = [vpnSubscriptionStatus, leoSubscriptionStatus].compactMap({ $0 }).first?
        .renewalInfo
    else {
      // There is currently no subscription so check if there was a restored receipt
      if Bundle.main.appStoreReceiptURL?.lastPathComponent == "sandboxReceipt" {
        return .sandbox
      }

      return .production
    }

    // Retrieve the current environment from StoreKit's Renewal Information
    switch renewalInfo {
    case .verified(let renewalInfo), .unverified(let renewalInfo, _):
      return .init(rawValue: renewalInfo.environment.rawValue) ?? .production
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

  /// A boolean indicating whether or not all the Origin product offerings has been loaded
  public var isOriginProductsLoaded: Bool {
    if originMonthlyProduct != nil || originYearlyProduct != nil {
      return true
    }

    return false
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
          case .originMonthly, .originYearly:
            originSubscriptionStatus = await transaction.subscriptionStatus
          }

          // Update SkusSDK
          do {
            try await self.updateSkusPurchaseState(for: product)
            didRestore = true
          } catch {
            Logger.module.error(
              "[BraveStoreSDK] - Failed to restore purchased product receipt: \(error, privacy: .public)"
            )
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
      Logger.module.error(
        "[BraveStoreSDK] - Failed to restore purchased product receipt: \(error, privacy: .public)"
      )
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
      if try await super.purchase(subscription) != nil {
        Logger.module.info("[BraveStoreSDK] - Product Purchase Successful")
      }
    }
  }

  /// Processes the product purchase transaction with the SkusService
  /// If the transaction cannot be processed (receipt is empty or null), throw an exception
  /// - Parameter productId: The ID of the product that is currently being purchased
  override public func processPurchase(of productId: Product.ID) async throws {
    // Find the Brave offered product from the AppStore Product ID
    guard let product = BraveStoreProduct.allCases.first(where: { productId == $0.rawValue })
    else {
      Logger.module.info("[BraveStoreSDK] - Not a Brave Product! - \(productId, privacy: .public)")
      throw BraveStoreSDKError.invalidProduct
    }

    // Update Skus SDK Purchase
    try await self.updateSkusPurchaseState(for: product)

    Logger.module.info("[BraveStoreSDK] - Purchase Successful")
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

    // Update origin products
    originMonthlyProduct = products.first(where: {
      $0.id == BraveStoreProduct.originMonthly.rawValue
    })
    originYearlyProduct = products.first(where: { $0.id == BraveStoreProduct.originYearly.rawValue }
    )
  }

  /// Observer function called when AppStore customer purchased products have been fetched or updated
  private func onPurchasesUpdated(_ products: Products) {
    // Process only subscriptions at this time as Brave has no other products
    let products = products.all.filter({ $0.type == .autoRenewable }).filter({
      $0.subscription != nil
    })

    // No products to process
    if products.isEmpty {
      return
    }

    Task { @MainActor [weak self] in
      guard let self = self else { return }

      // Retrieve subscriptions
      let vpnSubscriptions = products.filter({
        $0.id == BraveStoreProduct.vpnMonthly.rawValue
          || $0.id == BraveStoreProduct.vpnYearly.rawValue
      })

      let leoSubscriptions = products.filter({
        $0.id == BraveStoreProduct.leoMonthly.rawValue
          || $0.id == BraveStoreProduct.leoYearly.rawValue
      })

      let originSubscriptions = products.filter({
        $0.id == BraveStoreProduct.originMonthly.rawValue
          || $0.id == BraveStoreProduct.originYearly.rawValue
      })

      // Retrieve subscription statuses
      let vpnSubscriptionStatuses = vpnSubscriptions.compactMap({ $0.subscription })
      let leoSubscriptionsStatuses = leoSubscriptions.compactMap({ $0.subscription })
      let originSubscriptionsStatuses = originSubscriptions.compactMap({ $0.subscription })

      // Statuses apply to the entire group
      vpnSubscriptionStatus = try? await vpnSubscriptionStatuses.first?.status.first
      leoSubscriptionStatus = try? await leoSubscriptionsStatuses.first?.status.first
      originSubscriptionStatus = try? await originSubscriptionsStatuses.first?.status.first

      // Save subscription Ids
      Preferences.AIChat.subscriptionProductId.value = leoSubscriptions.first?.id
      Preferences.BraveOrigin.subscriptionProductId.value = originSubscriptions.first?.id

      // Once our backend allows restoring purchases `without` linking, we can get rid of this and just use `processTransaction`.
      #if BACKEND_SUPPORTS_IOS_MULTI_DEVICE_RESTORE

      // Restore product subscription if necessary
      if Preferences.AIChat.subscriptionOrderId.value == nil
        || Preferences.BraveOrigin.subscriptionOrderId.value == nil
      {
        // We don't have a cached subscriptionOrderId
        // This means the product was purchased on a different device
        // So let's automatically restore it to this device as well
        _ = await restorePurchases()
      }
      #endif
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

    guard let skusService else {
      throw SkusError.skusServiceUnavailable
    }

    Logger.module.info("[BraveStoreSDK] - Refreshing Receipt")

    // Attempt to update the Application Bundle's receipt, if necessary
    try await AppStoreReceipt.sync()

    // Create an order for the AppStore receipt
    // If an order already exists, refreshes the order information
    if let orderId = Preferences.AIChat.subscriptionOrderId.value, productGroup == .leo {
      try await skusService.refreshOrder(orderId: orderId, for: productGroup)
      return
    }

    if let orderId = Preferences.BraveOrigin.subscriptionOrderId.value, productGroup == .origin {
      try await skusService.refreshOrder(orderId: orderId, for: productGroup)
      return
    }

    Logger.module.info("[BraveStoreSDK] - No Order To Refresh")
    throw SkusError.cannotCreateOrder
  }

  /// Updates the Skus-SDK credentials and order information
  /// - Parameter product: The product whose information to update
  /// - Throws: An exception if updating the purchase information fails
  @MainActor
  private func updateSkusPurchaseState(for product: BraveStoreProduct) async throws {
    // This SDK currently only supports Leo
    // until we update the VPN code to use it
    switch product.group {
    case .vpn:
      return
    case .leo:
      Preferences.AIChat.subscriptionProductId.value = product.rawValue
    case .origin:
      Preferences.BraveOrigin.subscriptionProductId.value = product.rawValue
    }

    guard let skusService else {
      throw SkusError.skusServiceUnavailable
    }

    Logger.module.info("[BraveStoreSDK] - Syncing Receipt")

    // Attempt to update the Application Bundle's receipt, by force
    try await AppStoreReceipt.sync()

    if try AppStoreReceipt.receipt.isEmpty {
      Logger.module.error("[BraveStoreSDK] - Receipt is Empty")
      throw AppStoreReceipt.AppStoreReceiptError.invalidReceiptData
    }

    // Create a Skus-SDK for the specified product
    // Create an order for the AppStore receipt
    // If an order already exists, refreshes the order information
    let orderId = try await skusService.createOrder(for: product)

    // There is an order, and an expiry date, but no credentials
    // Fetch the credentials
    try await skusService.fetchCredentials(orderId: orderId, for: product.group)

    // Store the Order-ID
    switch product.group {
    case .vpn:
      break
    case .leo:
      Preferences.AIChat.subscriptionProductId.value = orderId
    case .origin:
      Preferences.BraveOrigin.subscriptionProductId.value = orderId
    }

    Logger.module.info("[BraveStoreSDK] - Order Completed")
  }
}
