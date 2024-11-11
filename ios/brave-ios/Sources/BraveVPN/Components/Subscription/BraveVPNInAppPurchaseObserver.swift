// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Shared
import StoreKit
import os.log

public class BraveVPNIAPObserverManager: NSObject, ObservableObject {

  public enum BraveVPNPaymentStatus: Equatable {
    case ongoing
    case success(receiptValidationRequired: Bool)
    case failure(_ error: BraveVPNInAppPurchaseObserver.PurchaseError?)
    case unknown
  }

  @Published var paymentStatus: BraveVPNPaymentStatus = .unknown
  @Published var isPromotedPurchase = false

  private let iapObserver: BraveVPNInAppPurchaseObserver

  public init(iapObserver: BraveVPNInAppPurchaseObserver) {
    self.iapObserver = iapObserver
    super.init()
    self.iapObserver.delegate = self
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
    isPromotedPurchase = true
  }
}

public protocol BraveVPNInAppPurchaseObserverDelegate: AnyObject {
  func purchasedOrRestoredProduct(validateReceipt: Bool)
  func purchaseFailed(error: BraveVPNInAppPurchaseObserver.PurchaseError)
  func handlePromotedInAppPurchase()
}

public class BraveVPNInAppPurchaseObserver: NSObject, SKPaymentTransactionObserver {

  public enum PurchaseError: Equatable {
    case transactionError(error: SKError?)
    case receiptError
  }

  public weak var delegate: BraveVPNInAppPurchaseObserverDelegate?
  public var savedPayment: SKPayment?

  // MARK: - Handling transactions

  private func processTransactions(_ transactions: [SKPaymentTransaction], on queue: SKPaymentQueue)
  {
    // This helper variable helps to call the IAPObserverDelegate delegate purchased method only once.
    // Reason is when restoring or sometimes when purchasing or restoring a product there's multiple transactions
    // that are returned in `transactions` array.
    // Apple advices to call `finishTransaction` for all of them,
    // but to show the UI we only want to call the delegate method once.
    var callPurchaseDelegateOnce = true

    // Filter for VPN only transactions. We do not want to call `finishTransaction` on other transactions
    let vpnTransactions = transactions.filter({
      ($0.payment.productIdentifier == BraveVPNProductInfo.ProductIdentifiers.monthlySub)
        || ($0.payment.productIdentifier == BraveVPNProductInfo.ProductIdentifiers.yearlySub)
    })

    // There was no VPN purchases
    if vpnTransactions.isEmpty {
      let errorRestore = SKError(SKError.unknown, userInfo: ["detail": "not-purchased"])
      self.delegate?.purchaseFailed(error: .transactionError(error: errorRestore))
      return
    }

    vpnTransactions
      .sorted(by: { $0.transactionDate ?? Date() > $1.transactionDate ?? Date() })
      .forEach { transaction in
        switch transaction.transactionState {
        case .purchased:
          Logger.module.debug("Received transaction state: purchased")
          // This should be always called, no matter if transaction is successful or not.
          queue.finishTransaction(transaction)
          if callPurchaseDelegateOnce {
            Preferences.VPN.subscriptionProductId.value = transaction.payment.productIdentifier
            self.delegate?.purchasedOrRestoredProduct(validateReceipt: true)
          }
          callPurchaseDelegateOnce = false
        case .restored:
          Logger.module.debug("Received transaction state: restored")
          // This should be always called, no matter if transaction is successful or not.
          queue.finishTransaction(transaction)

          if callPurchaseDelegateOnce {
            Preferences.VPN.subscriptionProductId.value = transaction.payment.productIdentifier

            Task {
              do {
                let response = try await BraveVPN.validateReceiptData()
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
              } catch {
                Logger.module.error("Error validating receipt: \(error)")
                self.delegate?.purchaseFailed(error: .transactionError(error: SKError(.unknown)))
              }
            }
          }

          callPurchaseDelegateOnce = false
        case .purchasing, .deferred:
          Logger.module.debug("Received transaction state: purchasing")
        case .failed:
          Logger.module.debug("Received transaction state: failed")
          queue.finishTransaction(transaction)
          if callPurchaseDelegateOnce {
            self.delegate?.purchaseFailed(
              error: .transactionError(error: transaction.error as? SKError)
            )
          }
          callPurchaseDelegateOnce = false
        @unknown default:
          assertionFailure("Unknown transactionState")
        }
      }
  }

  public func paymentQueue(
    _ queue: SKPaymentQueue,
    updatedTransactions transactions: [SKPaymentTransaction]
  ) {
    processTransactions(transactions, on: queue)
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
      return
    }

    processTransactions(queue.transactions, on: queue)
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
      productIdentifier == BraveVPNProductInfo.ProductIdentifiers.monthlySub
        || productIdentifier == BraveVPNProductInfo.ProductIdentifiers.yearlySub
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

extension BraveVPNInAppPurchaseObserver {
  @MainActor
  static func refreshReceipt() async throws {
    let request = SKReceiptRefreshRequest()
    let delegate = ReceiptRefreshDelegate()
    request.delegate = delegate

    try await withCheckedThrowingContinuation { continuation in
      delegate.completion = { result in
        switch result {
        case .success:
          continuation.resume()
        case .failure(let error):
          continuation.resume(throwing: error)
        }
      }

      request.start()
    }
  }

  private class ReceiptRefreshDelegate: NSObject, SKRequestDelegate {
    var completion: ((Result<Void, Error>) -> Void)?

    func requestDidFinish(_ request: SKRequest) {
      completion?(.success(()))
      completion = nil
      request.delegate = nil
    }

    func request(_ request: SKRequest, didFailWithError error: Error) {
      completion?(.failure(error))
      completion = nil
      request.delegate = nil
    }
  }
}
