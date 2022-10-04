// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A model used to decode JSON data we receive from Guardian's VPN-alerts endpoint.
/// This data is persisted in `BraveVPNAlert` mode.
public struct BraveVPNAlertJSONModel: Decodable {

  enum CodingKeys: String, CodingKey {
    case uuid, action, category, host, message, timestamp, title
  }

  public let uuid: String
  public let action: BraveVPNAlert.Action
  public let category: BraveVPNAlert.TrackerType
  public let host: String
  public let message: String
  public let timestamp: Int64
  public let title: String

  public init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)

    self.uuid = try container.decode(String.self, forKey: .uuid)

    let hostString = try container.decode(String.self, forKey: .host)

    // If possible we only add the base domain part of the host.
    // Reason is we do the same for regular ad blocking resources
    // This makes it easier to group together and show total counts for vpn + regular blocked resources.
    // https scheme is added only to make the `baseDomain` helper to return a non nil value.
    self.host = URL(string: "https://" + hostString)?.baseDomain ?? hostString
    self.message = try container.decode(String.self, forKey: .message)
    self.title = try container.decode(String.self, forKey: .title)

    // The VPN alerts array we receive is pretty spammy. It often consists of multiple requests
    // from the same domain, with almost the same timestamp.
    // This is because multiple resources are loaded by pages and the VPN detects them all.
    // In order to avoid having many duplicates, 'seconds' are cleared from the timestamp.
    // Further logic to clear out duplicates happens at the CoreData model level.
    let decodedTimestamp = Int64(try container.decode(Int.self, forKey: .timestamp))
    self.timestamp = decodedTimestamp - decodedTimestamp % 60

    let actionString = try container.decode(String.self, forKey: .action)
    switch actionString {
    case "drop":
      self.action = .drop
    case "log":
      self.action = .log
    default:
      throw DecodingError.dataCorrupted(.init(codingPath: [CodingKeys.action], debugDescription: "Casting `action` failed, incorrect value: \(actionString)"))
    }

    let categoryString = try container.decode(String.self, forKey: .category)
    switch categoryString {
    case "privacy-tracker-mail":
      self.category = .mail
    case "privacy-tracker-app-location":
      self.category = .location
    case "privacy-tracker-app":
      self.category = .app
    default:
      throw DecodingError.dataCorrupted(.init(codingPath: [CodingKeys.category], debugDescription: "Casting `category` failed, incorrect value: \(categoryString)"))
    }
  }
}
