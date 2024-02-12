// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
@testable import BraveVPN
import BraveShared
import XCTest
import GuardianConnect
import Preferences

class BraveVPNTests: XCTestCase {

  override func setUp() {

    let lineItemPurchasedNotInTrial: NSDictionary = [
      "quantity": "1",
      "expires_date": "2024-07-27 22:19:43 Etc/GMT",
      "expires_date_pst": "2024-07-27 15:19:43 America/Los_Angeles",
      "is_in_intro_offer_period": "false",
      "purchase_date_ms": "1690492783000",
      "transaction_id": "2000000377681042",
      "is_trial_period": "false",
      "original_transaction_id": "2000000159090000",
      "in_app_ownership_type": "PURCHASED",
      "original_purchase_date_pst": "2022-09-20 08:12:56 America/Los_Angeles",
      "product_id": "bravevpn.yearly",
      "purchase_date": "2024-07-27 22:19:43 Etc/GMT",
      "subscription_group_identifier": "20621968",
      "original_purchase_date_ms": "1663686776000",
      "expires_date_ms": "1690496383000",
      "purchase_date_pst": "2023-07-27 14:19:43 America/Los_Angeles",
      "original_purchase_date": "2022-09-20 15:12:56 Etc/GMT"]
    
    let lineItemPurchasedInTrial: NSDictionary = [
      "quantity": "1",
      "expires_date": "2024-07-27 22:19:43 Etc/GMT",
      "expires_date_pst": "2024-07-27 15:19:43 America/Los_Angeles",
      "is_in_intro_offer_period": "false",
      "purchase_date_ms": "1690492783000",
      "transaction_id": "2000000377681042",
      "is_trial_period": "true",
      "original_transaction_id": "2000000159090000",
      "in_app_ownership_type": "PURCHASED",
      "original_purchase_date_pst": "2022-09-20 08:12:56 America/Los_Angeles",
      "product_id": "bravevpn.yearly",
      "purchase_date": "2024-07-27 22:19:43 Etc/GMT",
      "subscription_group_identifier": "20621968",
      "original_purchase_date_ms": "1663686776000",
      "web_order_line_item_id": "2000000032896443",
      "expires_date_ms": "1690496383000",
      "purchase_date_pst": "2023-07-27 14:19:43 America/Los_Angeles",
      "original_purchase_date": "2022-09-20 15:12:56 Etc/GMT"]
    
    let lineItemMetaDataAutoRenewEnabled: NSDictionary = [
      "product_id": "bravevpn.yearly",
      "original_transaction_id": "2000000159090000",
      "auto_renew_product_id": "bravevpn.yearly",
      "auto_renew_status": "1"]
    
    let lineItemMetaDataAutoRenewCanceled: NSDictionary = [
      "product_id": "bravevpn.yearly",
      "original_transaction_id": "2000000159090000",
      "auto_renew_product_id": "bravevpn.yearly",
      "auto_renew_status": "0"]
    
    let lineItemMetaDataIsInRetryPeriod: NSDictionary = [
      "product_id": "bravevpn.yearly",
      "original_transaction_id": "2000000159090000",
      "auto_renew_product_id": "bravevpn.yearly",
      "auto_renew_status": "0",
      "is_in_billing_retry_period": "true"]
    
    let gracePeriod = 16 // days
    let modifiedDateWithGracePeriod = Calendar.current.date(byAdding: .day, value: gracePeriod, to: Date())!
    let gracePeriodExpiryInMs = Int(modifiedDateWithGracePeriod.timeIntervalSince1970 * 1000)

    let lineItemMetaDataIsInRetryPeriodWithGraceExpiry: NSDictionary = [
      "product_id": "bravevpn.yearly",
      "original_transaction_id": "2000000159090000",
      "auto_renew_product_id": "bravevpn.yearly",
      "auto_renew_status": "0",
      "grace_period_expires_date_ms": String(gracePeriodExpiryInMs),
      "is_in_billing_retry_period": "true"]
    

    subjectActivePeriodRenewableNotInTrial = generateReceiptResponse(using: lineItemPurchasedNotInTrial, metaData: lineItemMetaDataAutoRenewEnabled)
    subjectActivePeriodRenewableInTrial = generateReceiptResponse(using: lineItemPurchasedInTrial, metaData: lineItemMetaDataAutoRenewEnabled)
    subjectActivePeriodNotRenewable = generateReceiptResponse(using: lineItemPurchasedNotInTrial, metaData: lineItemMetaDataAutoRenewCanceled)
    subjectRetryPeriod = generateReceiptResponse(using: lineItemPurchasedNotInTrial, metaData: lineItemMetaDataIsInRetryPeriod)
    subjectRetryPeriodNoMetaGraceExpiry = generateReceiptResponse(using: nil, metaData: lineItemMetaDataIsInRetryPeriodWithGraceExpiry)
    subjectExpiredPeriod = generateReceiptResponse(using: nil, metaData: lineItemMetaDataAutoRenewCanceled)
  }

  override func tearDown() {
    subjectActivePeriodRenewableNotInTrial = nil
    subjectActivePeriodRenewableInTrial = nil
    subjectActivePeriodNotRenewable = nil
    subjectRetryPeriod = nil
    subjectExpiredPeriod = nil
    
    super.tearDown()
    
    Preferences.VPN.expirationDate.reset()
    Preferences.VPN.gracePeriodExpirationDate.reset()
  }

  func testSubscriptionActiveAutoRenewEnabledNotInTrial() {
    let processedLineItem = BraveVPN.processReceiptResponse(receiptResponseItem: subjectActivePeriodRenewableNotInTrial)
    
    XCTAssertTrue(processedLineItem.status == .active)
    XCTAssertTrue(processedLineItem.autoRenewEnabled)
    XCTAssertFalse(processedLineItem.isInTrialPeriod)
  }
  
  func testSubscriptionActiveAutoRenewEnabledInTrial() {
    let processedLineItem = BraveVPN.processReceiptResponse(receiptResponseItem: subjectActivePeriodRenewableInTrial)
    
    XCTAssertTrue(processedLineItem.status == .active)
    XCTAssertTrue(processedLineItem.autoRenewEnabled)
    XCTAssertTrue(processedLineItem.isInTrialPeriod)
  }
  
  func testSubscriptionActiveAutoRenewDisabled() {
    let processedLineItem = BraveVPN.processReceiptResponse(receiptResponseItem: subjectActivePeriodNotRenewable)
    
    XCTAssertTrue(processedLineItem.status == .active)
    XCTAssertFalse(processedLineItem.autoRenewEnabled)
  }
  
  func testSubscriptionIsInRetryPeriod() {
    let processedLineItem = BraveVPN.processReceiptResponse(receiptResponseItem: subjectRetryPeriod)
    
    XCTAssertTrue(processedLineItem.status == .retryPeriod)
    XCTAssertFalse(processedLineItem.autoRenewEnabled)
    XCTAssertFalse(processedLineItem.isInTrialPeriod)
  }
  
  func testSubscriptionIsInNoMetaGraceExpiry() {
    let processedLineItem = BraveVPN.processReceiptResponse(receiptResponseItem: subjectRetryPeriodNoMetaGraceExpiry)
    
    XCTAssertTrue(processedLineItem.status == .retryPeriod)
    XCTAssertFalse(processedLineItem.autoRenewEnabled)
    XCTAssertFalse(processedLineItem.isInTrialPeriod)
  }
  
  func testSubscriptionExpiredPeriod() {
    let processedLineItem = BraveVPN.processReceiptResponse(receiptResponseItem: subjectExpiredPeriod)
    
    XCTAssertTrue(processedLineItem.status == .expired)
    XCTAssertFalse(processedLineItem.autoRenewEnabled)
    XCTAssertFalse(processedLineItem.isInTrialPeriod)
  }
  
  private func generateReceiptResponse(using lineItem: NSDictionary?, metaData: NSDictionary)-> GRDIAPReceiptResponse {
    var receiptLineItem: GRDReceiptLineItem?
    
    if let lineItem = lineItem {
      receiptLineItem = GRDReceiptLineItem(
        dictionary: lineItem as! [AnyHashable: Any])
    }
    let receiptLineItemMetaData: GRDReceiptLineItemMetadata = GRDReceiptLineItemMetadata(
      dictionary: metaData as! [AnyHashable: Any])
    
    let receiptResponse = GRDIAPReceiptResponse(withReceiptResponse: ["":""])
    if let receiptLineItem = receiptLineItem {
      receiptResponse.lineItems = [receiptLineItem]
    } else {
      receiptResponse.lineItems = []
    }
    receiptResponse.lineItemsMetadata = [receiptLineItemMetaData]
    
    return receiptResponse
  }

  private var subjectActivePeriodRenewableNotInTrial: GRDIAPReceiptResponse!
  private var subjectActivePeriodRenewableInTrial: GRDIAPReceiptResponse!
  private var subjectActivePeriodNotRenewable: GRDIAPReceiptResponse!
  private var subjectRetryPeriod: GRDIAPReceiptResponse!
  private var subjectRetryPeriodNoMetaGraceExpiry: GRDIAPReceiptResponse!
  private var subjectExpiredPeriod: GRDIAPReceiptResponse!
}
