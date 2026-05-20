// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Shared
import os.log

// https://github.com/brave/brave-core/blob/master/components/skus/browser/rs/lib/src/models.rs#L137

/// A structure representing the customer's credentials
/// Returned by credentialsSummary
public struct SkusCredentialSummary: Codable {
  public let order: SkusOrder
  public let remainingCredentialCount: UInt
  public let expiresAt: Date?
  public let active: Bool
  public let nextActiveAt: Date?
}

/// A structure representing the customer's order information
/// Returned by refreshOrder
public struct SkusOrder: Codable {
  public let id: String
  public let createdAt: Date
  public let currency: String
  public let updatedAt: Date
  public let totalPrice: Double
  public let location: String
  public let merchantId: String
  public let status: String
  public let expiresAt: Date?
  public let lastPaidAt: Date?
  public let items: [SkusOrderItem]

  /// A structure representing a Line-Item within the customer's order
  /// An order can contain multiple items
  public struct SkusOrderItem: Codable {
    public let id: String
    public let orderId: String
    public let sku: String  // brave-leo-premium
    public let createdAt: Date
    public let updatedAt: Date
    public let currency: String
    public let quantity: Int
    public let price: Double
    public let subTotal: Double
    public let location: String  // leo.bravesoftware.com
    public let productDescription: String
    public let credentialType: SkusCredentialType

    enum CodingKeys: String, CodingKey {
      case id
      case orderId
      case sku
      case createdAt
      case updatedAt
      case currency
      case quantity
      case price
      case subTotal = "subtotal"
      case location
      case productDescription = "description"
      case credentialType
    }

    /// A structure representing the customer's credential type
    public enum SkusCredentialType: String, Codable {
      case singleUse = "single-use"
      case timeLimited = "time-limited"
      case timeLimitedv2 = "time-limited-v2"
    }
  }
}

/// An enum representing the Skus SDK Environment
public enum BraveSkusEnvironment {
  case beta
  case nightly
  case release

  /// Returns the current Skus SDK Environment
  public static var current: BraveSkusEnvironment {
    switch AppConstants.buildChannel {
    case .beta: return .beta
    case .debug, .nightly: return .nightly
    case .release: return .release
    }
  }
}

/// An error related to Skus handling
public enum SkusError: Error {
  /// The SkusService failed is unavailable for use
  /// Can be thrown due to SkusServiceFactory returning null
  case skusServiceUnavailable

  /// The Application's BundleID is invalid
  /// Can be thrown when encoding AppStore receipts
  case invalidBundleId

  /// The URL of the receipt stored in the Application Bundle is null or invalid
  case invalidReceiptURL

  /// The receipt is invalid
  /// Thrown when Skus cannot validate the receipt and fetch credentials
  case invalidReceiptData

  /// The receipt cannot be encoded/serialized for use with Skus
  case cannotEncodeReceipt

  /// The SDK was unable to create a purchase order or retrieve an existing order
  case cannotCreateOrder

  /// The SDK was unable to submit a receipt
  case cannotSubmitReceipt

  /// The SDK was unable to refresh an order
  case cannotRefreshOrder

  ///  The SDK was unable to fetch the customer's purchase credential summary
  case cannotFetchCredentialSummary

  ///  The SDK was unable to fetch the customer's purchase credentials
  case cannotFetchCredentials

  ///  The SDK was unable to prepare the customer's purchase credentials
  case cannotPrepareCredentials

  /// There was an error decoding an SDK response
  /// Can be thrown when the SDK fails to decode an order, order summary, credentials, etc
  case decodingError
}

extension JSONDecoder {
  /// A custom JSON Decoder that handles decoding Skus Object dates as ISO-8601
  /// with optional milli-seconds
  fileprivate static let skus: JSONDecoder = {
    let formatter = ISO8601DateFormatter()
    formatter.formatOptions = [
      .withYear,
      .withMonth,
      .withDay,
      .withTime,
      .withDashSeparatorInDate,
      .withColonSeparatorInTime,
    ]

    let decoder = JSONDecoder()
    decoder.keyDecodingStrategy = .convertFromSnakeCase
    decoder.dateDecodingStrategy = .custom({ decoder in
      let container = try decoder.singleValueContainer()
      let dateString = try container.decode(String.self)

      guard let date = formatter.date(from: try container.decode(String.self)) else {
        throw DecodingError.dataCorruptedError(
          in: container,
          debugDescription: "Cannot decode date string \(dateString)"
        )
      }

      return date
    })
    return decoder
  }()
}

extension SkusSkusService {
  /// Creates an order from an AppStore Receipt
  /// If an order already exists, returns the existing Order-ID
  /// - Parameter product: The purchased product to create an order for
  /// - Returns: The Order-ID associated with the AppStore receipt
  @MainActor
  public func createOrder(for product: BraveStoreProduct) async throws -> String {
    Logger.module.info("[SkusService] - Creating Order")

    let receipt = try AppStoreReceipt.receipt(for: product)

    Logger.module.info("[SkusService] - Fetched Receipt")

    Logger.module.info("[SkusService] - Creating Order From Receipt")
    let skusResult = await createOrderFromReceipt(
      domain: product.group.skusDomain,
      receipt: receipt
    )

    if skusResult.code != Skus.SkusResultCode.ok {
      Logger.module.info("[SkusService] - No OrderID - \(skusResult.message, privacy: .public)")
      throw SkusError.cannotCreateOrder
    }

    let orderId = skusResult.message
    if orderId.isEmpty {
      Logger.module.info("[SkusService] - No OrderID")
      throw SkusError.cannotCreateOrder
    }

    Logger.module.info("[SkusService] - OrderID: \(orderId, privacy: .private(mask: .hash))")
    return orderId
  }

  /// Retrieves and refreshes the local cached order for the given Order-ID
  /// - Parameter orderId: The ID of the order to retrieve
  /// - Parameter group: The purchased product group the order belongs to
  /// - Returns: The order information for the given order
  /// - Throws: An exception if the order could not be found or decoded
  @MainActor
  @discardableResult
  public func refreshOrder(
    orderId: String,
    for group: BraveStoreProductGroup
  ) async throws -> SkusOrder {
    func decode(_ response: String) throws -> SkusOrder {
      if response == "{}" {
        throw SkusError.decodingError
      }

      guard let data = response.data(using: .utf8) else {
        throw SkusError.decodingError
      }

      return try JSONDecoder.skus.decode(SkusOrder.self, from: data)
    }

    Logger.module.info(
      "[SkusService] - Refreshing Order: \(orderId, privacy: .private(mask: .hash))"
    )

    let skusResult = await refreshOrder(domain: group.skusDomain, orderId: orderId)

    if skusResult.code != Skus.SkusResultCode.ok {
      Logger.module.info(
        "[SkusService] - Failed Refreshing Order - \(skusResult.message, privacy: .public)"
      )
      throw SkusError.cannotRefreshOrder
    }

    return try decode(skusResult.message)
  }

  /// Retrieves the Customer's Credentials Summary
  /// - Returns: The customer's credentials summary which includes their order information
  /// - Parameter group: The purchased product group whose credentials summary to fetch
  /// - Throws: An exception if the credentials could not be retrieved or decoded
  @MainActor
  public func credentialsSummary(
    for group: BraveStoreProductGroup
  ) async throws -> SkusCredentialSummary {
    func decode(_ response: String) throws -> SkusCredentialSummary {
      if response == "{}" {
        throw SkusError.decodingError
      }

      guard let data = response.data(using: .utf8) else {
        throw SkusError.decodingError
      }

      return try JSONDecoder.skus.decode(SkusCredentialSummary.self, from: data)
    }

    let skusResult = await credentialSummary(domain: group.skusDomain)
    if skusResult.code != Skus.SkusResultCode.ok {
      Logger.module.info(
        "[SkusService] - Failed Fetching CredentialSummary - \(skusResult.message, privacy: .public)"
      )
      throw SkusError.cannotFetchCredentialSummary
    }

    return try decode(skusResult.message)
  }
}
