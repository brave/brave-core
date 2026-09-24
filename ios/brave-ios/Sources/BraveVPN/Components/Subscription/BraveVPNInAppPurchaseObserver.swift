// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStore
import Foundation
import Preferences
import Shared
import StoreKit
import os.log

@MainActor
public class BraveVPNIAPObserverManager: ObservableObject {

  public enum BraveVPNPaymentStatus: Equatable {
    case ongoing
    case success(receiptValidationRequired: Bool)
    case failure(_ error: BraveVPNInAppPurchaseObserver.PurchaseError?)
    case unknown
  }

  @Published var paymentStatus: BraveVPNPaymentStatus = .unknown

  private let iapObserver: BraveVPNInAppPurchaseObserver

  public init(iapObserver: BraveVPNInAppPurchaseObserver) {
    self.iapObserver = iapObserver
    iapObserver.delegate = self
  }
}

// MARK: - IAPObserverDelegate
extension BraveVPNIAPObserverManager: BraveVPNInAppPurchaseObserverDelegate {
  public func purchasedOrRestoredProduct(validateReceipt: Bool) {
    paymentStatus = .success(receiptValidationRequired: validateReceipt)
  }

  public func purchaseFailed(error: BraveVPNInAppPurchaseObserver.PurchaseError) {
    paymentStatus = .failure(error)
  }

  public func handlePromotedInAppPurchase() {
    // No-op In app purchase promotion is handled on bvc
  }
}

@MainActor
public protocol BraveVPNInAppPurchaseObserverDelegate: AnyObject {
  func purchasedOrRestoredProduct(validateReceipt: Bool)
  func purchaseFailed(error: BraveVPNInAppPurchaseObserver.PurchaseError)
  func handlePromotedInAppPurchase()
}

@MainActor
public class BraveVPNInAppPurchaseObserver {

  public enum PurchaseError: Equatable {
    /// The user cancelled the purchase or the AppStore authentication
    case cancelled
    /// The purchase or restore failed with an error from the AppStore
    case transactionFailed
    /// The transaction succeeded but receipt validation failed afterwards
    case receiptError
    /// A restore was attempted but the account has never bought the product
    case nothingToRestore
  }

  public weak var delegate: (any BraveVPNInAppPurchaseObserverDelegate)?

  /// A promoted AppStore purchase that was deferred until onboarding completes
  public var savedPromotedProduct: Product?

  /// Transaction ids that have already been reported to the delegate.
  /// A single purchase can be delivered both through `Product.purchase` and `Transaction.updates`.
  private var reportedTransactionIDs = Set<UInt64>()

  private var transactionUpdatesTask: Task<Void, Never>?
  private var purchaseIntentsTask: Task<Void, Never>?

  public init() {
    transactionUpdatesTask = Task { [weak self] in
      for await update in Transaction.updates {
        await self?.handleTransactionUpdate(update)
      }
    }

    purchaseIntentsTask = Task { [weak self] in
      for await intent in PurchaseIntent.intents {
        await self?.handlePurchaseIntent(intent)
      }
    }

    // Process any transactions that were not finished before the app was last terminated
    Task { [weak self] in
      for await update in Transaction.unfinished {
        await self?.handleTransactionUpdate(update)
      }
    }
  }

  deinit {
    transactionUpdatesTask?.cancel()
    purchaseIntentsTask?.cancel()
  }

  // MARK: - Purchasing

  public func purchase(product: Product) async {
    do {
      let result = try await product.purchase(options: [.simulatesAskToBuyInSandbox(false)])

      switch result {
      case .success(let verificationResult):
        guard case .verified(let transaction) = verificationResult else {
          Logger.module.error("Purchase returned an unverified transaction")
          delegate?.purchaseFailed(error: .transactionFailed)
          return
        }

        let didNotify = await processPurchasedTransaction(transaction)
        if !didNotify {
          // The transaction was already processed, which happens when the user already owns
          // the subscription and it was delivered through Transaction.updates beforehand.
          // Notify the delegate again so callers waiting on this purchase can resolve.
          delegate?.purchasedOrRestoredProduct(validateReceipt: true)
        }
      case .userCancelled:
        // The user cancelled the purchase, no error should be surfaced
        delegate?.purchaseFailed(error: .cancelled)
      case .pending:
        // Deferred purchase (e.g. ask to buy), the transaction will arrive
        // through Transaction.updates once approved
        break
      @unknown default:
        assertionFailure("Unknown purchase result")
      }
    } catch {
      Logger.module.error("Purchase failed: \(error.localizedDescription)")
      delegate?.purchaseFailed(error: .transactionFailed)
    }
  }

  // MARK: - Restoring

  public func restorePurchases() async {
    do {
      try await AppStore.sync()
    } catch StoreKitError.userCancelled {
      // The user dismissed the AppStore authentication, no error should be surfaced
      delegate?.purchaseFailed(error: .cancelled)
      return
    } catch {
      Logger.module.error("Restoring purchases failed: \(error.localizedDescription)")
      delegate?.purchaseFailed(error: .transactionFailed)
      return
    }

    var latestTransaction: Transaction?
    for await result in Transaction.currentEntitlements {
      guard case .verified(let transaction) = result,
        isVPNProduct(transaction.productID),
        transaction.revocationDate == nil
      else { continue }

      if let current = latestTransaction, transaction.purchaseDate <= current.purchaseDate {
        continue
      }

      latestTransaction = transaction
    }

    guard let transaction = latestTransaction else {
      Logger.module.debug(
        "Restoring transaction failed - Nothing to restore - Account never bought this product"
      )
      delegate?.purchaseFailed(error: .nothingToRestore)
      return
    }

    Preferences.VPN.subscriptionProductId.value = transaction.productID

    // Receipt validation reads the receipt from the app bundle, which StoreKit 2 does not
    // reliably write after restoring a purchase
    try? await AppStoreReceipt.sync()

    do {
      let response = try await BraveVPN.validateReceiptData()
      if response?.status == .expired {
        // Receipt either expired or receipt validation returned some error.
        delegate?.purchaseFailed(error: .receiptError)
      } else {
        delegate?.purchasedOrRestoredProduct(validateReceipt: false)
        // If we purchased via Apple's IAP we reset the Brave SKUs credential
        // to avoid mixing two purchase types in the app.
        //
        // The user will be able to retrieve the shared credential
        // after log in to account.brave website.
        BraveVPN.clearSkusCredentials(includeExpirationDate: false)
      }
    } catch {
      Logger.module.error("Error validating receipt: \(error)")
      delegate?.purchaseFailed(error: .transactionFailed)
    }
  }

  // MARK: - Handling transactions

  private func handleTransactionUpdate(_ result: VerificationResult<Transaction>) async {
    guard case .verified(let transaction) = result else {
      Logger.module.error("Received an unverified transaction update")
      return
    }

    // Only handle VPN transactions, other products are handled by BraveStoreSDK
    guard isVPNProduct(transaction.productID) else { return }

    Logger.module.debug(
      "Received transaction update for \(transaction.productID, privacy: .public)"
    )

    await processPurchasedTransaction(transaction)
  }

  /// - Returns: True if the transaction was processed by this call, false if it was already processed
  @discardableResult
  private func processPurchasedTransaction(_ transaction: Transaction) async -> Bool {
    // A purchase is delivered through both `Product.purchase` and `Transaction.updates`,
    // but the delegate should only be notified once
    guard reportedTransactionIDs.insert(transaction.id).inserted else {
      Logger.module.debug("Transaction \(transaction.id) was already processed")
      return false
    }

    // StoreKit 2 does not reliably add the AppStore receipt to the app bundle after a
    // transaction, force a refresh so backend receipt validation can see the purchase
    try? await AppStoreReceipt.sync()

    Preferences.VPN.subscriptionProductId.value = transaction.productID
    delegate?.purchasedOrRestoredProduct(validateReceipt: true)
    await transaction.finish()
    return true
  }

  // MARK: - Handling promoted in-app purchases

  private func handlePurchaseIntent(_ intent: PurchaseIntent) async {
    let product = intent.product

    // Check the product triggered from ad is a VPN product
    // This check is done because this observer is used in browser
    guard isVPNProduct(product.id) else { return }

    // Check if there is an active onboarding happening
    // If you need to defer until onboarding is complete, save the product for later
    if Preferences.AppState.shouldDeferPromotedPurchase.value {
      savedPromotedProduct = product
      return
    }

    delegate?.handlePromotedInAppPurchase()
    await purchase(product: product)
  }

  private func isVPNProduct(_ productID: String) -> Bool {
    productID == BraveStoreProduct.vpnMonthly.rawValue
      || productID == BraveStoreProduct.vpnYearly.rawValue
  }
}
