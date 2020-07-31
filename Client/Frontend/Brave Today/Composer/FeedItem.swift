// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

@propertyWrapper struct URLString: Equatable, Decodable {
    var wrappedValue: URL?
    
    init(wrappedValue: URL?) {
        self.wrappedValue = wrappedValue
    }
    
    init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        if container.decodeNil() {
            wrappedValue = nil
        } else {
            let value = try container.decode(String.self)
            wrappedValue = URL(string: value)
        }
    }
}

struct FeedItem: Equatable, Comparable {
    var score: Double
    var content: Content
    var source: Source
    
    static func < (lhs: Self, rhs: Self) -> Bool {
        return lhs.score < rhs.score
    }
}

extension FeedItem {
    
    struct Source: Equatable, Decodable {
        var id: String
        var enabled: Bool
        var category: String
        @URLString var logo: URL?
        var name: String
        
        enum CodingKeys: String, CodingKey {
            case id = "publisher_id"
            case enabled
            case category
            case logo = "publisher_logo_padded"
            case name = "publisher_name"
        }
        
        static func == (lhs: Self, rhs: Self) -> Bool {
            return lhs.id == rhs.id
        }
    }
    
    struct FeedContentType: Decodable, Equatable {
        var rawValue: String
        init(rawValue: String) {
            self.rawValue = rawValue
        }
        init(from decoder: Decoder) throws {
            rawValue = try decoder.singleValueContainer().decode(String.self)
        }
        static let article = FeedContentType(rawValue: "article")
        static let deals = FeedContentType(rawValue: "product")
        static let sponsor = FeedContentType(rawValue: "brave_offers")
    }

    struct Content: Equatable, Decodable {
        var publishTime: Date
        @URLString var url: URL?
        @URLString var imageURL: URL?
        var title: String
        var description: String
        var contentType: FeedContentType
        var publisherID: String
        var urlHash: String
        
        enum CodingKeys: String, CodingKey {
            case publishTime = "publish_time"
            case url
            case imageURL = "padded_img"
            case title
            case description
            case contentType = "content_type"
            case publisherID = "publisher_id"
            case urlHash = "url_hash"
        }
    }
}
