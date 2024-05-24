// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Shared
import StoreKit
import os.log

public protocol BraveVPNInAppPurchaseObserverDelegate: AnyObject {
  func purchasedOrRestoredProduct(validateReceipt: Bool)
  func purchaseFailed(error: BraveVPNInAppPurchaseObserver.PurchaseError)
  func handlePromotedInAppPurchase()
}

public class BraveVPNInAppPurchaseObserver: NSObject, SKPaymentTransactionObserver {

  public enum PurchaseError {
    case transactionError(error: SKError?)
    case receiptError
  }

  public weak var delegate: BraveVPNInAppPurchaseObserverDelegate?
  public var savedPayment: SKPayment?

  // MARK: - Handling transactions

  public func paymentQueue(
    _ queue: SKPaymentQueue,
    updatedTransactions transactions: [SKPaymentTransaction]
  ) {
    // This helper variable helps to call the IAPObserverDelegate delegate purchased method only once.
    // Reason is when restoring or sometimes when purchasing or restoring a product there's multiple transactions
    // that are returned in `transactions` array.
    // Apple advices to call `finishTransaction` for all of them,
    // but to show the UI we only want to call the delegate method once.
    var callPurchaseDelegateOnce = true

    // For safety let's start processing from the newest transaction.
    transactions
      .filter({
        ($0.payment.productIdentifier == VPNProductInfo.ProductIdentifiers.monthlySub)
          || ($0.payment.productIdentifier == VPNProductInfo.ProductIdentifiers.yearlySub)
      })
      .sorted(by: { $0.transactionDate ?? Date() > $1.transactionDate ?? Date() })
      .forEach { transaction in
        switch transaction.transactionState {
        case .purchased:
          Logger.module.debug("Received transaction state: purchased")
          // This should be always called, no matter if transaction is successful or not.
          SKPaymentQueue.default().finishTransaction(transaction)
          if callPurchaseDelegateOnce {
            Preferences.VPN.subscriptionProductId.value = transaction.payment.productIdentifier
            self.delegate?.purchasedOrRestoredProduct(validateReceipt: true)
          }
          callPurchaseDelegateOnce = false
        case .restored:
          Logger.module.debug("Received transaction state: restored")
          // This should be always called, no matter if transaction is successful or not.
          SKPaymentQueue.default().finishTransaction(transaction)

          if callPurchaseDelegateOnce {
            Preferences.VPN.subscriptionProductId.value = transaction.payment.productIdentifier

            BraveVPN.validateReceiptData { [weak self] response in
              guard let self = self else { return }

              if response?.status == .expired {
                // Receipt either expired or receipt validation returned some error.
                self.delegate?.purchaseFailed(error: .receiptError)
              } else {
                self.delegate?.purchasedOrRestoredProduct(validateReceipt: false)
                // If we purchased via Apple's IAP we reset the Brave SKUs credential
                // to avoid mixing two purchase types in the app.
                //
                // The user will be able to retrieve the shared credential
                // after log in to account.brave website.
                BraveVPN.clearSkusCredentials(includeExpirationDate: false)
              }
            }
          }

          callPurchaseDelegateOnce = false
        case .purchasing, .deferred:
          Logger.module.debug("Received transaction state: purchasing")
        case .failed:
          Logger.module.debug("Received transaction state: failed")
          SKPaymentQueue.default().finishTransaction(transaction)
          self.delegate?.purchaseFailed(
            error: .transactionError(error: transaction.error as? SKError)
          )
        @unknown default:
          assertionFailure("Unknown transactionState")
        }
      }
  }

  // MARK: - Restoring Transactions

  public func paymentQueue(
    _ queue: SKPaymentQueue,
    restoreCompletedTransactionsFailedWithError error: Error
  ) {
    Logger.module.debug("Restoring transaction failed")
    self.delegate?.purchaseFailed(error: .transactionError(error: error as? SKError))
  }

  // Used to handle restoring transaction error for users never purchased but trying to restore
  public func paymentQueueRestoreCompletedTransactionsFinished(_ queue: SKPaymentQueue) {
    if queue.transactions.isEmpty {
      Logger.module.debug(
        "Restoring transaction failed - Nothing to restore - Account never bought this product"
      )

      let errorRestore = SKError(SKError.unknown, userInfo: ["detail": "not-purchased"])
      delegate?.purchaseFailed(error: .transactionError(error: errorRestore))
    }
  }

  // MARK: - Handling promoted in-app purchases

  public func paymentQueue(
    _ queue: SKPaymentQueue,
    shouldAddStorePayment payment: SKPayment,
    for product: SKProduct
  ) -> Bool {
    // Check the product triggered from ad is a VPN product
    // This check is done because this observer is used in browser
    let productIdentifier = product.productIdentifier
    guard
      productIdentifier == VPNProductInfo.ProductIdentifiers.monthlySub
        || productIdentifier == VPNProductInfo.ProductIdentifiers.yearlySub
    else {
      return false
    }

    // Check if there is an active onboarding happening
    let shouldDeferPayment = Preferences.AppState.shouldDeferPromotedPurchase.value

    // If you need to defer until onboarding is complete, save the payment and return false.
    if shouldDeferPayment {
      savedPayment = payment
      return false
    }

    delegate?.handlePromotedInAppPurchase()
    return true
  }
}
