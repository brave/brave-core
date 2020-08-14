// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A property wrapper over a optional URL primarly used to ensure we handle stringify'd URL's from JSON
/// without failing when they are empty strings or `null`
///
/// This wrapper decodes a `String` type rather than a `URL` type and converts that decoded string into a
/// `URL` using `URL(string:)`
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
