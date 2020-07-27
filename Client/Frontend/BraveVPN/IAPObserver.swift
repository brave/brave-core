// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import StoreKit
import Shared
import BraveShared

private let log = Logger.browserLogger

protocol IAPObserverDelegate: AnyObject {
    func purchasedOrRestoredProduct()
    func purchaseFailed(error: IAPObserver.PurchaseError)
}

class IAPObserver: NSObject, SKPaymentTransactionObserver {
    
    enum PurchaseError {
        case transactionError(error: SKError?)
        case receiptError
    }
    
    weak var delegate: IAPObserverDelegate?

    func paymentQueue(_ queue: SKPaymentQueue, updatedTransactions transactions: [SKPaymentTransaction]) {
            transactions.forEach { transaction in
                switch transaction.transactionState {
                case .purchased, .restored:
                    log.debug("Received transaction state: purchased or restored")
                    BraveVPN.validateReceipt() { [weak self] expired in
                        guard let self = self else { return }
                        // This should be always called, no matter if transaction is successful or not.
                        SKPaymentQueue.default().finishTransaction(transaction)
                        
                        if expired == false {
                            self.delegate?.purchasedOrRestoredProduct()
                        } else {
                            // Receipt either expired or receipt validation returned some error.
                            self.delegate?.purchaseFailed(error: .receiptError)
                        }
                    }
                case .purchasing, .deferred:
                    log.debug("Received transaction state: purchasing")
                case .failed:
                    log.debug("Received transaction state: failed")
                    self.delegate?.purchaseFailed(
                        error: .transactionError(error: transaction.error as? SKError))
                    SKPaymentQueue.default().finishTransaction(transaction)
                @unknown default:
                    assertionFailure("Unknown transactionState")
                }
            }
        }
                
    func paymentQueue(_ queue: SKPaymentQueue, restoreCompletedTransactionsFailedWithError error: Error) {
        log.debug("Restoring transaction failed")
        self.delegate?.purchaseFailed(error: .transactionError(error: error as? SKError))
    }
}
