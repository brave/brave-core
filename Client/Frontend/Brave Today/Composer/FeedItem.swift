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
    var isContentHidden: Bool
    
    static func < (lhs: Self, rhs: Self) -> Bool {
        return lhs.score < rhs.score
    }
}

extension FeedItem {
    
    struct Source: Equatable, Decodable {
        var enabled: Bool
        @URLString var logo: URL?
        
        enum CodingKeys: String, CodingKey {
            case enabled
            case logo = "publisher_logo_padded"
        }
    }

    struct Content: Equatable, Decodable {
        var category: String
        var publishTime: Date
        @URLString var url: URL?
        @URLString var imageURL: URL?
        var title: String
        var description: String
        var contentType: String
        var publisherID: String
        var publisherName: String
        var urlHash: String
        
        enum CodingKeys: String, CodingKey {
            case category
            case publishTime = "publish_time"
            case url
            case imageURL = "padded_img"
            case title
            case description
            case contentType = "content_type"
            case publisherID = "publisher_id"
            case publisherName = "publisher_name"
            case urlHash = "url_hash"
        }
    }
}
