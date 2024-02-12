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
@propertyWrapper public struct URLString: Hashable, Decodable {
  public var wrappedValue: URL?

  public init(wrappedValue: URL?) {
    self.wrappedValue = wrappedValue
  }

  public init(from decoder: Decoder) throws {
    let container = try decoder.singleValueContainer()
    if container.decodeNil() {
      wrappedValue = nil
    } else {
      let value = try container.decode(String.self)
      wrappedValue = URL(string: value)
    }
  }
}

extension KeyedDecodingContainer {
  /// URLString's wrapped value type is always an optional URL, therefore should be decoded as if the
  /// type itself was optional (via `decodeIfPresent` like the usual synthesized version)
  public func decode(
    _ type: URLString.Type,
    forKey key: KeyedDecodingContainer<K>.Key
  ) throws -> URLString {
    try decodeIfPresent(type, forKey: key) ?? .init(wrappedValue: nil)
  }
}
