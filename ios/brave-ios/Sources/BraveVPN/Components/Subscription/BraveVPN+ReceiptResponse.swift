// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import GuardianConnect
import Preferences

extension BraveVPN {

  public enum BraveVPNError: Error {
    case noReceipt
    case noReceiptData
    case noReceiptStatus
    case noReceiptExpiryDate
    case noReceiptGraceExpiryDate
    case noReceiptIsInTrialPeriod
    case noReceiptAutoRenewEnabled
    case noReceiptExpiryIntent
    case noReceiptExpiryIntentUnknown
    case noResponse
  }

  public struct ReceiptResponse {
    public enum Status: Int {
      case active, expired, retryPeriod
    }

    enum ExpirationIntent: Int {
      case none, cancelled, billingError, priceIncreaseConsent, notAvailable, unknown
    }

    var status: Status
    var expiryReason: ExpirationIntent = .none
    var expiryDate: Date?
    var graceExpiryDate: Date?

    var isInTrialPeriod: Bool = false
    var autoRenewEnabled: Bool = false
  }

  public static func loadReceipt() async -> String? {
    guard let receiptUrl = Bundle.main.appStoreReceiptURL,
      let receipt = try? Data(contentsOf: receiptUrl).base64EncodedString
    else { return nil }

    return receipt
  }

  /// Connects to Guardian's server to validate locally stored receipt.
  /// Returns ReceiptResponse which hold information about status of receipt expiration etc
  public static func validateReceiptData() async throws -> ReceiptResponse? {
    guard let receipt = await loadReceipt(),
      let bundleId = Bundle.main.bundleIdentifier
    else {
      throw BraveVPNError.noReceipt
    }

    if Preferences.VPN.skusCredential.value != nil {
      // Receipt verification applies to Apple's IAP only,
      // if we detect Brave's SKU token we should not look at Apple's receipt.
      return nil
    }

    return try await withCheckedThrowingContinuation { continuation in
      housekeepingApi.verifyReceiptData(receipt, bundleId: bundleId) { response, error in
        if let error = error {
          // Error while fetching receipt response, the variations of error can be listed
          // No App Store receipt data present
          // Failed to retrieve receipt data from server
          // Failed to decode JSON response data
          logAndStoreError("Call for receipt verification failed: \(error.localizedDescription)")
          continuation.resume(throwing: error)
          return
        }

        guard let response = response else {
          logAndStoreError("Receipt verification response is empty")
          continuation.resume(throwing: BraveVPNError.noResponse)
          return
        }

        let receiptResponseItem = GRDIAPReceiptResponse(withReceiptResponse: response)
        let processedReceiptDetail = BraveVPN.processReceiptResponse(
          receiptResponseItem: receiptResponseItem
        )

        switch processedReceiptDetail.status {
        case .expired:
          Preferences.VPN.expirationDate.value = Date(timeIntervalSince1970: 1)
          Preferences.VPN.originalTransactionId.value = nil
          logAndStoreError(
            "VPN Subscription LineItems are empty subscription expired",
            printToConsole: false
          )
        case .active, .retryPeriod:
          if let expirationDate = processedReceiptDetail.expiryDate {
            Preferences.VPN.expirationDate.value = expirationDate
          }

          if let gracePeriodExpirationDate = processedReceiptDetail.graceExpiryDate {
            Preferences.VPN.gracePeriodExpirationDate.value = gracePeriodExpirationDate
          }

          Preferences.VPN.freeTrialUsed.value = !processedReceiptDetail.isInTrialPeriod

          populateRegionDataIfNecessary()
          GRDSubscriptionManager.setIsPayingUser(true)
        }

        Preferences.VPN.vpnReceiptStatus.value = processedReceiptDetail.status.rawValue

        continuation.resume(returning: processedReceiptDetail)
      }
    }
  }

  public static func processReceiptResponse(
    receiptResponseItem: GRDIAPReceiptResponse
  ) -> ReceiptResponse {
    guard
      let newestReceiptLineItem = receiptResponseItem.lineItems.sorted(by: {
        $0.expiresDate > $1.expiresDate
      }).first
    else {
      if let originalTransactionId = Preferences.VPN.originalTransactionId.value {
        let lineItemMetaDataForOriginalId = receiptResponseItem.lineItemsMetadata.first(
          where: { Int($0.originalTransactionId) ?? 00 == originalTransactionId })

        if let metaData = lineItemMetaDataForOriginalId, metaData.gracePeriodExpiresDate > Date() {
          let response = ReceiptResponse(
            status: .retryPeriod,
            expiryReason: ReceiptResponse.ExpirationIntent(rawValue: Int(metaData.expirationIntent))
              ?? .none,
            graceExpiryDate: metaData.gracePeriodExpiresDate,
            isInTrialPeriod: false,
            autoRenewEnabled: false
          )

          return response
        }
      }

      return ReceiptResponse(status: .expired)
    }

    let lineItemMetaData = receiptResponseItem.lineItemsMetadata.first(
      where: { Int($0.originalTransactionId) ?? 00 == newestReceiptLineItem.originalTransactionId })

    // Original transaction id of last active subscription in order to detect grace period
    Preferences.VPN.originalTransactionId.value = newestReceiptLineItem.originalTransactionId

    guard let metadata = lineItemMetaData else {
      logAndStoreError("No line item meta data - can not happen")
      return ReceiptResponse(status: .active)
    }

    let receiptStatus: ReceiptResponse.Status =
      lineItemMetaData?.isInBillingRetryPeriod == true ? .retryPeriod : .active
    // Expiration Intent is unsigned
    let expirationIntent =
      ReceiptResponse.ExpirationIntent(rawValue: Int(metadata.expirationIntent)) ?? .none
    // 0 is for turned off renewal, 1 is subscription renewal
    let autoRenewEnabled = metadata.autoRenewStatus == 1

    let response = ReceiptResponse(
      status: receiptStatus,
      expiryReason: expirationIntent,
      expiryDate: newestReceiptLineItem.expiresDate,
      graceExpiryDate: metadata.gracePeriodExpiresDate,
      isInTrialPeriod: newestReceiptLineItem.isTrialPeriod,
      autoRenewEnabled: autoRenewEnabled
    )

    return response
  }
}
