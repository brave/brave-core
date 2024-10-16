// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import StoreKit
import os.log

/// An opaque product protocol representing an AppStore product
public protocol AppStoreProduct: RawRepresentable<String>, CaseIterable {
  var subscriptionGroup: String { get }
}

/// A class that handles all AppStore Receipt related requests
public actor AppStoreReceipt {
  private init() {}

  /// Retrieves the AppStore Purchase Receipt stored in the Application Bundle
  /// Returns the receipt as a Base64 encoded string
  public static var receipt: String {
    get throws {
      guard let receiptUrl = Bundle.main.appStoreReceiptURL else {
        Logger.module.info("[AppStoreReceipt] - Invalid Appstore Receipt URL")
        throw AppStoreReceiptError.invalidReceiptURL
      }

      do {
        return try Data(contentsOf: receiptUrl).base64EncodedString
      } catch {
        Logger.module.error(
          "[AppStoreReceipt] - Failed to retrieve AppStore Receipt: \(error.localizedDescription)"
        )
        throw AppStoreReceiptError.invalidReceiptData
      }
    }
  }

  /// Forces the AppStore to add the receipt to the Application Bundle
  /// When using StoreKit 2, receipts are no longer stored in the Application
  /// This function forces the AppStore to place it in the bundle. Once back-end services update to use Transactions API
  /// this function will be obsolete
  static func sync() async throws {
    Logger.module.info("[AppStoreReceipt] - Refreshing Receipt")

    let fetcher = AppStoreReceiptRefresher()
    return try await withCheckedThrowingContinuation { continuation in
      fetcher.refreshReceipt { error in
        DispatchQueue.main.async {
          if let error = error {
            Logger.module.error("[AppStoreReceipt] - Error Refreshing Receipt: \(error)")
            continuation.resume(throwing: error)
            return
          }

          Logger.module.info("[AppStoreReceipt] - Receipt Refreshed Successfully")
          continuation.resume()
        }
      }
    }
  }

  /// An AppStore Receipt Error
  enum AppStoreReceiptError: Error {
    /// No receipt found in the Application Bundle
    case invalidReceiptURL

    /// Failed to read the AppStore Receipt from the Application Bundle
    case invalidReceiptData
  }

  /// Forces the AppStore to add receipts to the Application Bundle.
  private class AppStoreReceiptRefresher: NSObject, SKRequestDelegate {
    private let request = SKReceiptRefreshRequest()
    private var onRefreshComplete: ((Error?) -> Void)?

    override init() {
      super.init()
      self.request.delegate = self
    }

    deinit {
      self.request.cancel()  // StoreKit background task leak fix
    }

    /// Triggered a refresh of the AppStore receipt
    func refreshReceipt(with listener: @escaping (Error?) -> Void) {
      if onRefreshComplete == nil {
        self.onRefreshComplete = listener
        self.request.start()
      }
    }

    /// Restore of receipt completed
    func requestDidFinish(_ request: SKRequest) {
      self.onRefreshComplete?(nil)
      self.onRefreshComplete = nil
      self.request.delegate = nil
      self.request.cancel()  // StoreKit background task leak fix
    }

    /// Restore of receipt failed
    func request(_ request: SKRequest, didFailWithError error: Error) {
      self.onRefreshComplete?(error)
      self.onRefreshComplete = nil
      self.request.delegate = nil
      self.request.cancel()  // StoreKit background task leak fix
    }
  }

  /// Forces the AppStore to add receipts to the Application Bundle, and also to restore In-App Transactions
  private class AppStoreTransactionRestorer: NSObject, SKPaymentTransactionObserver {
    private let queue = SKPaymentQueue()
    private var onRefreshComplete: ((Error?) -> Void)?

    override init() {
      super.init()
      self.queue.add(self)
    }

    /// Restores completed transactions which triggers an AppStore receipt refresh
    func restoreTransactions(with listener: @escaping (Error?) -> Void) {
      if onRefreshComplete == nil {
        self.onRefreshComplete = listener
        self.queue.restoreCompletedTransactions()
      }
    }

    func paymentQueue(
      _ queue: SKPaymentQueue,
      updatedTransactions transactions: [SKPaymentTransaction]
    ) {
      // No transactions to restore, so the receipt can be empty
      if transactions.isEmpty {
        onRefreshComplete?(SKError(.storeProductNotAvailable))
        onRefreshComplete = nil
        return
      }

      var completion: (() -> Void)?

      // Sort the transactions and restore the receipt
      transactions
        .sorted(using: KeyPathComparator(\.transactionDate, order: .reverse))
        .forEach { transaction in
          switch transaction.transactionState {
          case .purchased:
            if completion == nil {
              completion = { [weak self] in
                guard let self = self else { return }
                self.onRefreshComplete?(nil)
                self.onRefreshComplete = nil
              }
            }

            // Apple states that all processes transactions must be marked finish after processing
            self.queue.finishTransaction(transaction)

          case .restored:
            if completion == nil {
              completion = { [weak self] in
                guard let self = self else { return }
                self.onRefreshComplete?(nil)
                self.onRefreshComplete = nil
              }
            }

            // Apple states that all processes transactions must be marked finish after processing
            self.queue.finishTransaction(transaction)

          case .purchasing, .deferred:
            break

          case .failed:
            if completion == nil {
              completion = { [weak self] in
                guard let self = self else { return }
                self.onRefreshComplete?(SKError(.storeProductNotAvailable))
                self.onRefreshComplete = nil
              }
            }

            // Apple states that all processes transactions must be marked finish after processing
            self.queue.finishTransaction(transaction)

          @unknown default:
            break
          }
        }

      // Finished restoring the receipt
      self.queue.remove(self)
      completion?()
    }

    func paymentQueue(
      _ queue: SKPaymentQueue,
      restoreCompletedTransactionsFailedWithError error: Error
    ) {
      // Restoring the receipt failed
      self.onRefreshComplete?(error)
      self.onRefreshComplete = nil
      self.queue.remove(self)
    }
  }
}

/// A class using StoreKit 2, to handle transactions and purchases
public class AppStoreSDK: ObservableObject {
  /// A structure containing all products available to the customer
  public struct Products {
    /// Products the customer consumes once. Not a subscription product.
    public let consumable: [Product]

    /// Products the customer does not consume. Not a subscription product.
    public let nonConsumable: [Product]

    /// Products the customer purchased that cannot be automatically renewed. Not a subscription product.
    public let nonRenewable: [Product]

    /// Products the customer purchased as a subscription that automatically renews. A subscription product.
    public let autoRenewable: [Product]

    /// All products the user has purchased or was given.
    public var all: [Product] {
      return consumable + nonConsumable + nonRenewable + autoRenewable
    }

    init() {
      self.consumable = []
      self.nonConsumable = []
      self.nonRenewable = []
      self.autoRenewable = []
    }

    init(
      consumable: [Product],
      nonConsumable: [Product],
      nonRenewable: [Product],
      autoRenewable: [Product]
    ) {
      self.consumable = consumable
      self.nonConsumable = nonConsumable
      self.nonRenewable = nonRenewable
      self.autoRenewable = autoRenewable
    }
  }

  /// Returns all available productson the store
  /// Each store must implement this function and return the list of products available
  /// - Returns: A list of products available in the store
  public var allAppStoreProducts: [any AppStoreProduct] {
    fatalError("[AppStoreSDK] - Not Implemented")
  }

  /// Products Available on the AppStore
  @Published
  private(set) public var allProducts = Products()

  /// Products the customer has purchased
  @Published
  private(set) public var purchasedProducts = Products()

  // MARK: - Private

  /// Task that tracks AppStore product updates
  private var updateTask: Task<Void, Error>?

  public init() {
    // Start updater immediately
    updateTask = Task.detached {
      for await result in Transaction.updates {
        Logger.module.info("[AppStoreSDK] - Transaction Update Received")

        do {
          // Verify the transaction
          let transaction = try self.verify(result)

          // Once our backend allows restoring purchases `without` linking, we can get rid of this and just use `processTransaction`.
          #if !BACKEND_SUPPORTS_IOS_MULTI_DEVICE_RESTORE
          // Retrieve all products the user purchased
          let purchasedProducts = await self.fetchPurchasedProducts()

          // Transactions must be marked as completed once processed
          await transaction.finish()

          // Distribute the purchased products to the customer
          await MainActor.run {
            self.purchasedProducts = purchasedProducts
          }
          #else
          // Process the transaction with our backend
          try await self.processTransaction(transaction, for: transaction.productID)
          #endif
        } catch {
          Logger.module.error(
            "[AppStoreSDK] - Transaction Verification Failed: \(error, privacy: .public)"
          )
        }
      }
    }

    // Fetch initial products immediately
    Task.detached {
      // Retrieve all products the user purchased
      await self.fetchProducts()

      // Retrieve all products the user purchased
      let purchasedProducts = await self.fetchPurchasedProducts()

      // Distribute the purchased products to the customer
      await MainActor.run {
        self.purchasedProducts = purchasedProducts
      }
    }
  }

  deinit {
    // Stop listening to AppStore product updates
    updateTask?.cancel()
  }

  // MARK: - Public

  /// Retrieves a product's renewable subscription product
  /// - Parameter product: The product whose SKU to retrieve
  /// - Returns: The AppStore renewable subscription product
  public func subscription(for product: any AppStoreProduct) async -> Product? {
    allProducts.autoRenewable.first(where: { $0.id == product.rawValue })
  }

  /// Retrieves a product's renewable subscription status
  /// - Parameter product: The product whose subscription status to retrieve
  /// - Returns: The renewable subscription's status
  public func status(for product: any AppStoreProduct) async -> [Product.SubscriptionInfo.Status] {
    (try? await subscription(for: product)?.subscription?.status) ?? []
  }

  /// Retrieves a product's renewable subscription trial status
  /// - Parameter product: The product whose subscription trial status to retrieve
  /// - Returns: The renewable subscription's trial status
  public func isIntroOfferAvailable(for product: any AppStoreProduct) async -> Bool {
    guard let subscription = await subscription(for: product)?.subscription else {
      return false
    }

    if subscription.introductoryOffer == nil {
      return false
    }

    return await subscription.isEligibleForIntroOffer == true
  }

  /// Retrieves a product's renewable subscription renewal information
  /// - Parameter product: The product whose renewal to retrieve
  /// - Returns: The renewable subscription's renewal information
  public func renewalState(
    for product: any AppStoreProduct
  ) async -> Product.SubscriptionInfo.RenewalState? {
    await status(for: product).first?.state
  }

  /// Retrieves a product's transaction history
  /// - Parameter product: The product whose last transaction history to retrieve
  /// - Returns: The product's latest transaction history
  public func latestTransaction(for product: any AppStoreProduct) async -> Transaction? {
    if let transaction = await Transaction.latest(for: product.rawValue) {
      do {
        return try verify(transaction)
      } catch {
        Logger.module.error(
          "[AppStoreSDK] - Transaction Verification Failed: \(error, privacy: .public)"
        )
      }
    }

    return nil
  }

  /// Retrieves a customer's purchase entitlement of a specified product
  /// - Parameter product: The product whose current subscription to retrieve
  /// - Returns: The current product entitlement/purchase. Null if the customer is not currently entitled to this product
  public func currentTransaction(for product: any AppStoreProduct) async -> Transaction? {
    if let transaction = await Transaction.currentEntitlement(for: product.rawValue) {
      do {
        return try verify(transaction)
      } catch {
        Logger.module.error(
          "[AppStoreSDK] - Transaction Verification Failed: \(error, privacy: .public)"
        )
      }
    }

    return nil
  }

  /// Purchases a product using In-App Purchases
  /// - Parameter product: The product customer wants to purchase
  /// - Returns: Returns the validated purchase transaction. Returns null if the transaction failed
  public func purchase(_ product: Product) async throws -> Transaction? {
    let result = try await product.purchase(options: [.simulatesAskToBuyInSandbox(false)])
    switch result {
    case .success(let result):
      // If we cannot force a receipt to be added to the app,
      // leave the transaction pending.
      try await AppStoreReceipt.sync()

      let receipt = try AppStoreReceipt.receipt
      if receipt.isEmpty {
        Logger.module.info("[AppStoreSDK] - Receipt is NOT valid!")
        return nil
      }

      if let data = Data(base64Encoded: receipt),
        let receipt = BraveStoreKitReceipt(data: data),
        receipt.inAppPurchaseReceipts.isEmpty
      {
        Logger.module.info("[AppStoreSDK] - Receipt is Empty!")
        return nil
      }

      // Verify the transaction
      let transaction = try self.verify(result)

      // Process the transaction with our backend
      try await processTransaction(transaction, for: product.id)

      // Return the processed transaction
      return transaction

    case .userCancelled, .pending:
      // The transaction is pending
      // Do nothing with the transaction
      return nil

    @unknown default:
      // Some future states for transactions have been added
      // Do nothing with the transaction
      Logger.module.error("[AppStoreSDK] - Unknown Purchase State")
      return nil
    }
  }

  /// Determines if the customer has currently purchased a product and is entitled to it.
  /// - Parameter product: The product whose purchase status to check
  /// - Returns: True if the customer is entitled to this product. False if the customer did not purchase the specified product
  public func isPurchased(_ product: Product) async throws -> Bool {
    switch product.type {
    case .consumable:
      return allProducts.consumable.contains(product)

    case .nonConsumable:
      return allProducts.nonConsumable.contains(product)

    case .nonRenewable:
      return allProducts.nonRenewable.contains(product)

    case .autoRenewable:
      return allProducts.autoRenewable.contains(product)

    default:
      Logger.module.error(
        "[AppStoreSDK] - Unknown Product Type: \(product.type.rawValue, privacy: .public)"
      )
      return false
    }
  }

  /// - Parameter productId: The ID of the product that is currently being purchased
  public func processPurchase(of productId: Product.ID) async throws {
    fatalError("[AppStoreSDK] - ProcessPurchase Not Implemented")
  }

  // MARK: - Private

  /// Verifies a transaction/purchase on device.
  /// - Parameter result: The verification status of the Transaction or Product
  /// - Throws: Throws an exception if verification failed
  /// - Returns: Returns the transaction or product if the verification was successful
  private func verify<T>(_ result: VerificationResult<T>) throws -> T {
    Logger.module.info("[AppStoreSDK] - Verifying Transaction")

    switch result {
    case .unverified(_, let error):
      Logger.module.info("[AppStoreSDK] - Failed Verifying Transaction: \(error)")
      throw error

    case .verified(let signedType):
      Logger.module.info("[AppStoreSDK] - Transaction Verified")
      return signedType
    }
  }

  /// Processes a transaction and adds a receipt to our Bundle
  /// - Parameter transaction: The verified transaction to process
  /// - Parameter productId: The ID of the product or line item
  /// - Throws: Throws an exception if processing fails for any reason
  private func processTransaction(
    _ transaction: Transaction,
    for productId: Product.ID
  ) async throws {
    Logger.module.info("[AppStoreSDK] - Processing Transaction: \(productId, privacy: .public)")

    // Retrieve all products the user purchased
    let purchasedProducts = await self.fetchPurchasedProducts()

    // Do additional purchase processing such as server-side validation
    // This function also asks for a receipt refresh
    try await processPurchase(of: productId)

    Logger.module.info("[AppStoreSDK] - Marking Transaction Completed")

    // If there is an error, do NOT call `transaction.finish`
    // For non-consumable products, it can be called.
    // For consumable products, they are removed from the receipt FOREVER and it should never be called,
    // until we can guarantee it is delivered to the user.
    // Transactions must be marked as completed once processed
    await transaction.finish()

    Logger.module.info("[AppStoreSDK] - Distributing Purchase To User")

    // Distribute the purchased products to the customer
    await MainActor.run {
      self.purchasedProducts = purchasedProducts
    }
  }

  /// Fetches all available products from the AppStore
  private func fetchProducts() async {
    do {
      var consumable = [Product]()
      var nonConsumable = [Product]()
      var nonRenewable = [Product]()
      var autoRenewable = [Product]()

      let products = try await Product.products(for: allAppStoreProducts.map({ $0.rawValue }))
      for product in products {
        switch product.type {
        case .consumable:
          consumable.append(product)

        case .nonConsumable:
          nonConsumable.append(product)

        case .nonRenewable:
          nonRenewable.append(product)

        case .autoRenewable:
          autoRenewable.append(product)

        default:
          Logger.module.error(
            "[AppStoreSDK] - Fetched Product of Unknown Type: \(product.type.rawValue, privacy: .public)"
          )
          break
        }
      }

      let availableProducts = Products(
        consumable: consumable,
        nonConsumable: nonConsumable,
        nonRenewable: nonRenewable,
        autoRenewable: autoRenewable
      )
      await MainActor.run {
        self.allProducts = availableProducts
      }
    } catch {
      Logger.module.error(
        "[AppStoreSDK] - Unable to fetch AppStore Products: \(error, privacy: .public)"
      )
    }
  }

  /// Fetches all the customer's purchased products from the AppStore
  /// - Returns: The list of products the customer is entitled to -- Products the customer has purchased
  private func fetchPurchasedProducts() async -> Products {
    var consumable = [Product]()
    var nonConsumable = [Product]()
    var nonRenewable = [Product]()
    var autoRenewable = [Product]()

    for await result in Transaction.currentEntitlements {
      do {
        let transaction = try verify(result)

        switch transaction.productType {
        case .consumable:
          if let product = allProducts.consumable.first(where: { $0.id == transaction.productID }) {
            consumable.append(product)
          }
          break

        case .nonConsumable:
          if let product = allProducts.nonConsumable.first(where: { $0.id == transaction.productID }
          ) {
            nonConsumable.append(product)
          }

        case .nonRenewable:
          if let product = allProducts.nonRenewable.first(where: { $0.id == transaction.productID })
          {
            // We can also filter non-renewable subscriptions by transaction.purchaseDate
            nonRenewable.append(product)
          }

        case .autoRenewable:
          if let product = allProducts.autoRenewable.first(where: { $0.id == transaction.productID }
          ) {
            autoRenewable.append(product)
          }

        default:
          Logger.module.error(
            "[AppStoreSDK] - Fetched Product of Unknown Type: \(transaction.productType.rawValue)"
          )
          break
        }
      } catch {
        Logger.module.error(
          "[AppStoreSDK] - Transaction Verification Failed: \(error, privacy: .public)"
        )
      }
    }

    return Products(
      consumable: consumable,
      nonConsumable: nonConsumable,
      nonRenewable: nonRenewable,
      autoRenewable: autoRenewable
    )
  }
}
