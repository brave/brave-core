// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

private let log = Logger.browserLogger

class BraveSkusWebHelper {
  /// On which hosts the receipt should be allowed to be exposed.
  private let allowedHosts = ["account.brave.com", "account.bravesoftware.com", "account.brave.software"]
  /// Which parameters have to be present before we expose the iOS receipt.
  private let requiredQueryItems: [URLQueryItem] =
  [.init(name: "intent", value: "connect-receipt"), .init(name: "product", value: "vpn")]
  /// What key should be used to pass the receipt in session storage.
  private let storageKey = "braveVpn.receipt"
  /// Value to pass to the json file's 'subscription_id' property. This is not related to the IAPs product ID.
  private let receiptJsonSubscriptionId = "brave-firewall-vpn-premium"
  
  private let url: URL
  
  /// Optional constructor. Returns nil if nothing should be injected to the page.
  /// Checks for few conditions like proper host and query parameters.
  init?(for url: URL) {
    guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false),
            let host = components.host,
            let scheme = components.scheme,
            let queryItems = components.queryItems else {
      return nil
    }

    if !allowedHosts.contains(host) || scheme != "https" { return nil }

    // Both query parameters must be present before the receipt is allowed to be shown.
    if requiredQueryItems.allSatisfy(queryItems.contains) {
      self.url = url
    } else {
      return nil
    }
  }
  
  fileprivate var receipt: String? {
    guard let receiptUrl = Bundle.main.appStoreReceiptURL else { return nil }
    
    do {
      return try Data(contentsOf: receiptUrl).base64EncodedString
    } catch {
      log.error("Failed to encode or get receipt data: \(error)")
      return nil
    }
  }
  
  /// Returns app's receipt and few other properties as a base64 encoded JSON.
  var receiptData: (key: String, value: String)? {
    guard let receipt = receipt, let bundleId = Bundle.main.bundleIdentifier else { return nil }
    
    struct ReceiptDataJson: Codable {
      let type: String
      let rawReceipt: String
      let package: String
      let subscriptionId: String
      
      enum CodingKeys: String, CodingKey {
        case type, package
        case rawReceipt = "raw_receipt"
        case subscriptionId = "subscription_id"
      }
    }
    
    let json = ReceiptDataJson(type: "ios",
                               rawReceipt: receipt,
                               package: bundleId,
                               subscriptionId: receiptJsonSubscriptionId)
    
    do {
      return (key: storageKey, value: try JSONEncoder().encode(json).base64EncodedString)
    } catch {
      assertionFailure("serialization error: \(error)")
      return nil
    }
  }
}

// MARK: - Testing
class BraveSkusWebHelperMock: BraveSkusWebHelper {
  static let mockReceiptValue = "test-receipt"
  
  override var receipt: String? {
    Self.mockReceiptValue
  }
}
