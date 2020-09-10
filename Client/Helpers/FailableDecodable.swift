// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A decodable class whos wrapped value can fail to decode without causing the containing Decodable to fail
///
/// For example we may have some JSON that contains an array of Foo objects like:
///
///     [{"x": 1}, {"x": null}]
///
/// And it's represented Decodable type:
///
///     struct Foo: Decodable {
///         var x: Int
///     }
///
/// Decoding this with FailableDecodable like so
///
///     // Results in [Foo(1), nil]
///     let bar = JSONDecoder().decode([FailableDecodable<Foo>].self)
///         .map(\.wrappedValue)
///
/// will decode successfully with the result of `[Foo(1), nil]` instead of causing the entire decoding to fail
/// due to an invalid value.
///
/// You can also be used as a property within a decodable container as a property wrapper, for example:
///
///     struct Foo: Decodable {
///         @FailableDecodable var x: Bar?
///     }
///
/// where `Bar` is some complex type
@propertyWrapper struct FailableDecodable<T: Decodable>: Decodable {
    var wrappedValue: T?
    init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        #if MOZ_CHANNEL_DEBUG
        // In debug builds we print out failed decodes to console so we can fix the issue or notify the
        // appropriate team about some malformed JSON
        do {
            wrappedValue = try container.decode(T.self)
        } catch {
            print("FailableDecodable failed to decode to type \(T.self): \(error)")
            wrappedValue = nil
        }
        #else
        wrappedValue = try? container.decode(T.self)
        #endif
    }
}
