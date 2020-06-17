// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

struct FeedSource: Decodable {
    var isDefault: Bool
    var id: String
    @URLString var logo: URL?
    var name: String
    
    enum CodingKeys: String, CodingKey {
        case isDefault = "default"
        case id = "publisher_id"
        case logo = "publisher_logo"
        case name = "publisher_name"
    }
}
