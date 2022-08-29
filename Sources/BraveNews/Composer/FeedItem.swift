// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CodableHelpers

public struct FeedItem: Equatable, Comparable {
  public var score: Double
  public var content: Content
  public var source: Source

  public static func < (lhs: Self, rhs: Self) -> Bool {
    lhs.score < rhs.score
  }
}

extension FeedItem {

  public struct Source: Equatable, Decodable {
    public var id: String
    public var isDefault: Bool
    public var category: String
    public var name: String
    public var isUserSource = false

    enum CodingKeys: String, CodingKey {
      case id = "publisher_id"
      case isDefault = "enabled"
      case category
      case name = "publisher_name"
    }

    public static func == (lhs: Self, rhs: Self) -> Bool {
      lhs.id == rhs.id
    }
  }

  public struct FeedContentType: Decodable, Equatable {
    var rawValue: String
    init(rawValue: String) {
      self.rawValue = rawValue
    }
    public init(from decoder: Decoder) throws {
      rawValue = try decoder.singleValueContainer().decode(String.self)
    }
    public static let article = FeedContentType(rawValue: "article")
    public static let deals = FeedContentType(rawValue: "product")
    public static let sponsor = FeedContentType(rawValue: "brave_offers")
    public static let partner = FeedContentType(rawValue: "brave_partner")
  }

  public struct Content: Equatable, Decodable {
    public var publishTime: Date
    @URLString public var url: URL?
    @URLString public var imageURL: URL?
    public var title: String
    public var description: String
    public var contentType: FeedContentType
    public var publisherID: String
    public var urlHash: String
    public var baseScore: Double
    public var offersCategory: String?
    public var creativeInstanceID: String?

    enum CodingKeys: String, CodingKey {
      case publishTime = "publish_time"
      case url
      case imageURL = "padded_img"
      case title
      case description
      case contentType = "content_type"
      case publisherID = "publisher_id"
      case urlHash = "url_hash"
      case baseScore = "score"
      case offersCategory = "offers_category"
      case creativeInstanceID = "creative_instance_id"
    }
  }
}
