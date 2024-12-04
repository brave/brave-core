// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import os.log

// swift-format-ignore
/// For adding a sample to an enumerated histogram
public func UmaHistogramEnumeration<E: RawRepresentable & CaseIterable>(
  _ name: String,
  sample: E
) where E.RawValue == Int {
  UmaHistogramExactLinear(name, sample.rawValue, E.allCases.count + 1)
}

/// A bucket that may span a single value or a range of values
///
/// Essentially a type eraser around `RangeExpression`
public struct Bucket {
  public var contains: (Int) -> Bool
  public static func equals(_ value: Int) -> Self {
    .init(contains: { value == $0 })
  }
  public static func r(_ value: some RangeExpression<Int>) -> Self {
    .init(contains: { value.contains($0) })
  }
}

extension Bucket: ExpressibleByIntegerLiteral {
  public init(integerLiteral value: IntegerLiteralType) {
    self = .equals(value)
  }
}

// swift-format-ignore
/// Adds a sample to a specific bucket. The answer will be the index of the bucket the value falls into.
///
/// Examples:
///   `UmaHistogramRecordValueToBucket("", buckets: [0, .r(1..<10), 10, .r(11...)], value: 0)` would answer 0
///   `UmaHistogramRecordValueToBucket("", buckets: [0, .r(1..<10), 10, .r(11...)], value: 4)` would answer 1
///   `UmaHistogramRecordValueToBucket("", buckets: [0, .r(1..<10), 10, .r(11...)], value: 10)` would answer 2
///   `UmaHistogramRecordValueToBucket("", buckets: [0, .r(1..<10), 10, .r(11...)], value: 21)` would answer 3
public func UmaHistogramRecordValueToBucket(
  _ name: String,
  buckets: [Bucket],
  value: Int
) {
  guard let answer = buckets.firstIndex(where: { $0.contains(value) }) else {
    Logger.module.warning("Value (\(value)) not found in any bucket for histogram \(name)")
    return
  }
  UmaHistogramExactLinear(name, answer, buckets.count + 1)
}

// swift-format-ignore
/// Adds a sample to record info around the last time a feature was used.
///
/// By default this uses the 7 standard buckets used in many P3A questions but alternative ones can be passed
/// in to use if needed.
public func UmaHistogramRecordLastFeatureUsage(
  _ name: String,
  option: Preferences.Option<Date?>,
  alternativeBuckets: [Bucket]? = nil
) {
  let calendar = Calendar(identifier: .gregorian)
  guard let lastUsageDate = option.value,
    let numberOfDays = calendar.dateComponents(
      [.day],
      from: lastUsageDate,
      to: Date()
    ).day
  else {
    return
  }
  let buckets: [Bucket] =
    alternativeBuckets ?? [
      .r(0...6),
      .r(7...13),
      .r(14...20),
      .r(21...27),
      .r(28...59),
      .r(60...),
    ]
  UmaHistogramRecordValueToBucket(name, buckets: buckets, value: numberOfDays)
}
