// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CodableHelpers

public struct FeedItem: Identifiable, Hashable, Comparable {
  public var score: Double
  public var content: Content
  public var source: Source

  public static func < (lhs: Self, rhs: Self) -> Bool {
    lhs.score < rhs.score
  }
  
  public func hash(into hasher: inout Hasher) {
    hasher.combine(content.urlHash)
    hasher.combine(source.id)
  }
  
  public var id: String {
    "\(content.urlHash)-\(source.id)"
  }
  
  public static func == (lhs: Self, rhs: Self) -> Bool {
    lhs.source.id == rhs.source.id && lhs.content.urlHash == rhs.content.urlHash
  }
}

extension FeedItem {
  
  public struct SourceSimilarity: Equatable, Decodable {
    public var sourceID: String
    public var relativeScore: Double
    
    enum CodingKeys: String, CodingKey {
      case sourceID = "source"
      case relativeScore = "score"
    }
  }
  
  public struct Source: Hashable, Decodable, Identifiable {
    public var id: String
    public var isDefault: Bool?
    public var category: String
    public var name: String
    public var isUserSource = false
    @URLString public var siteURL: URL?
    @URLString public var feedURL: URL?
    @URLString public var faviconURL: URL?
    @URLString public var coverURL: URL?
    public var destinationDomains: [String]
    public var backgroundColor: String?
    public var localeDetails: [LocaleDetails]?
    
    public func rank(of locale: String) -> Int {
      localeDetails?.first(where: { $0.locale == locale })?.rank ?? Int.max
    }
    
    enum CodingKeys: String, CodingKey {
      case id = "publisher_id"
      case isDefault = "enabled"
      case category
      case name = "publisher_name"
      case destinationDomains = "destination_domains"
      case siteURL = "site_url"
      case feedURL = "feed_url"
      case faviconURL = "favicon_url"
      case coverURL = "cover_url"
      case backgroundColor = "background_color"
      case localeDetails = "locales"
    }
    
    public func hash(into hasher: inout Hasher) {
      hasher.combine(id)
    }
    
    public static func == (lhs: Self, rhs: Self) -> Bool {
      lhs.id == rhs.id
    }
    
    public struct LocaleDetails: Hashable, Decodable {
      public var channels: Set<String>
      public var locale: String
      public var rank: Int?
    }
  }

  public struct LegacySource: Equatable, Decodable {
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

  public struct FeedContentType: Decodable, Hashable {
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

  public struct Content: Hashable, Decodable {
    public var publishTime: Date
    @URLString public var url: URL?
    @URLString public var imageURL: URL?
    public var title: String
    public var description: String
    public var contentType: FeedContentType
    public var publisherID: String
    public var urlHash: String
    public var baseScore: Double?
    public var offersCategory: String?
    public var creativeInstanceID: String?
    
    public func hash(into hasher: inout Hasher) {
      hasher.combine(urlHash)
      hasher.combine(publisherID)
    }
    
    public static func == (lhs: Self, rhs: Self) -> Bool {
      lhs.urlHash == rhs.urlHash && lhs.publisherID == rhs.publisherID
    }

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
