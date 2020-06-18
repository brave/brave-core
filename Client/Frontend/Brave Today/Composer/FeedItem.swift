// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

@propertyWrapper struct URLString: Equatable, Decodable {
    var wrappedValue: URL?
    
    init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        let value = try container.decode(String.self)
        wrappedValue = URL(string: value)
    }
}

struct FeedItem: Equatable, Decodable {
    var category: String
    var publishTime: Date
    @URLString var url: URL?
    var domain: String?
    @URLString var imageURL: URL?
    var title: String
    var description: String
    var contentType: String
    var publisherID: String
    var publisherName: String
    @URLString var publisherLogo: URL?
    
    enum CodingKeys: String, CodingKey {
        case category
        case publishTime = "publish_time"
        case url
        case domain
        case imageURL = "padded_img"
        case title
        case description
        case contentType = "content_type"
        case publisherID = "publisher_id"
        case publisherName = "publisher_name"
        case publisherLogo = "publisher_logo"
    }
}
