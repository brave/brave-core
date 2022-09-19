// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import StoreKit
import Shared
import BraveShared

private let log = Logger.browserLogger

public protocol IAPObserverDelegate: AnyObject {
  func purchasedOrRestoredProduct(validateReceipt: Bool)
  func purchaseFailed(error: IAPObserver.PurchaseError)
}

public class IAPObserver: NSObject, SKPaymentTransactionObserver {

  public enum PurchaseError {
    case transactionError(error: SKError?)
    case receiptError
  }

  public weak var delegate: IAPObserverDelegate?

  public func paymentQueue(_ queue: SKPaymentQueue, updatedTransactions transactions: [SKPaymentTransaction]) {
    // This helper variable helps to call the IAPObserverDelegate delegate purchased method only once.
    // Reason is when restoring or sometimes when purchasing or restoring a product there's multiple transactions
    // that are returned in `transactions` array.
    // Apple advices to call `finishTransaction` for all of them,
    // but to show the UI we only want to call the delegate method once.
    var callPurchaseDelegateOnce = true
    
    // For safety let's start processing from the newest transaction.
    transactions
      .sorted(by: { $0.transactionDate ?? Date() > $1.transactionDate ?? Date() })
      .forEach { transaction in
      switch transaction.transactionState {
      case .purchased:
        log.debug("Received transaction state: purchased")
        // This should be always called, no matter if transaction is successful or not.
        SKPaymentQueue.default().finishTransaction(transaction)
        if callPurchaseDelegateOnce {
          self.delegate?.purchasedOrRestoredProduct(validateReceipt: true)
        }
        callPurchaseDelegateOnce = false
      case .restored:
        log.debug("Received transaction state: restored")
        // This should be always called, no matter if transaction is successful or not.
        SKPaymentQueue.default().finishTransaction(transaction)
        
        if callPurchaseDelegateOnce {
          BraveVPN.validateReceipt() { [weak self] expired in
            guard let self = self else { return }
            
            if expired == false {
              self.delegate?.purchasedOrRestoredProduct(validateReceipt: false)
              // If we purchased via Apple's IAP we reset the Brave SKUs credential
              // to avoid mixing two purchase types in the app.
              //
              // The user will be able to retrieve the shared credential
              // after log in to account.brave website.
              Preferences.VPN.skusCredential.reset()
              Preferences.VPN.skusCredentialDomain.reset()
            } else {
              // Receipt either expired or receipt validation returned some error.
              self.delegate?.purchaseFailed(error: .receiptError)
            }
          }
        }
        
        callPurchaseDelegateOnce = false
      case .purchasing, .deferred:
        log.debug("Received transaction state: purchasing")
      case .failed:
        log.debug("Received transaction state: failed")
        SKPaymentQueue.default().finishTransaction(transaction)
        self.delegate?.purchaseFailed(
          error: .transactionError(error: transaction.error as? SKError))
      @unknown default:
        assertionFailure("Unknown transactionState")
      }
    }
  }

  public func paymentQueue(_ queue: SKPaymentQueue, restoreCompletedTransactionsFailedWithError error: Error) {
    log.debug("Restoring transaction failed")
    self.delegate?.purchaseFailed(error: .transactionError(error: error as? SKError))
  }
}
