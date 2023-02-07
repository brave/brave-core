// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import os.log
import BraveShared

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
  var contains: (Int) -> Bool
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

/// Adds a sample to record info around the last time a feature was used.
///
/// By default this uses the 7 standard buckets used in many P3A questions but alternative ones can be passed
/// in to use if needed.
public func UmaHistogramRecordLastFeatureUsage(
  _ name: String,
  option: Preferences.Option<Date?>,
  alternativeBuckets: [Bucket]? = nil
) {
  let calendar = Calendar.current
  guard let lastUsageDate = option.value,
        let numberOfDays = calendar.dateComponents(
          [.day],
          from: lastUsageDate,
          to: Date()
        ).day
  else {
    return
  }
  let buckets: [Bucket] = alternativeBuckets ?? [
    .r(0...6),
    .r(7...13),
    .r(14...20),
    .r(21...27),
    .r(28...59),
    .r(60...)
  ]
  UmaHistogramRecordValueToBucket(name, buckets: buckets, value: numberOfDays)
}

public struct P3AFeatureUsage {
  public let name: String
  public let histogram: String
  
  public let firstUsageOption: Preferences.Option<Date?>
  public let lastUsageOption: Preferences.Option<Date?>
  
  public init(name: String, histogram: String) {
    self.name = name
    self.histogram = histogram
    self.firstUsageOption = .init(key: "p3a.feature-usage.\(name).first", default: nil)
    self.lastUsageOption = .init(key: "p3a.feature-usage.\(name).last", default: nil)
  }
  
  public func recordHistogram() {
    // Record last usage to bucket if there's an associated p3a question
    UmaHistogramRecordLastFeatureUsage(histogram, option: lastUsageOption)
  }
  
  public func recordUsage() {
    // Update usage prefs
    let calendar = Calendar.current
    let now = Date()
    if firstUsageOption.value == nil {
      firstUsageOption.value = calendar.startOfDay(for: now)
    }
    lastUsageOption.value = calendar.startOfDay(for: now)
    recordHistogram()
  }
}

/// Storage for abritrary time-based P3A questions.
///
/// Each instance of timed storage should be created with a unique name
///
/// Event storage has a given event lifetime before an event is removed from storage as well as rules around
/// adding/removing events
public struct P3ATimedStorage<Value: Codable> {
  
  /// A single instance of data tied to some date & time
  public struct Record: Codable, Identifiable {
    /// A unique ID tied to the record
    public var id: UUID
    /// The date that the record was added
    public var date: Date
    /// The value recorded
    public var value: Value
  }
  
  /// A unique name to identify what data this storage container is holding.
  ///
  /// This name will be used to persist data
  public let name: String
  
  /// A list of records added
  private(set) public var records: [Record] = []
  
  /// The number of days that data in this container is valid for before it is purged
  private let lifetimeInDays: Int
  
  /// The current date
  ///
  /// Can be used in tests to override the date
  public var date: () -> Date = { Date() } {
    didSet {
      // Adjusting the date in a test should purge data. Data is purged on ``init(name:lifetimeInDays:)``
      // and ``append(value:)`` which would be the common use before accessing ``records``. Since changing the
      // date can cause records to become invalid, we need to purge when it changes too in tests.
      purge()
    }
  }
  
  /// The storage container persisting data for this p3a question
  private let storage: Preferences.Option<Data>
  
  /// Creates a storage container to hold onto P3A data for up to a given number of days
  public init(name: String, lifetimeInDays: Int) {
    self.name = name
    self.lifetimeInDays = lifetimeInDays
    self.storage = .init(key: "p3a.event-storage.\(name)", default: .init())
    
    if let data = try? JSONDecoder().decode([Record].self, from: storage.value) {
      records = data
    }
    purge()
  }
  
  fileprivate var cutoffTime: Date {
    Calendar.current.date(
      byAdding: .day,
      value: -lifetimeInDays,
      to: Calendar.current.startOfDay(for: date())
    ) ?? Date()
  }
  
  /// Purges any records that are outside of the lifetime
  private mutating func purge() {
    records.removeAll(where: { [cutoffTime] in $0.date <= cutoffTime })
  }
  
  /// Resets the storage container
  public mutating func reset() {
    storage.reset()
    records = []
  }
  
  /// Adds a record to the list
  public mutating func append(value: Value) {
    defer {
      save()
    }
    records.append(.init(id: .init(), date: Calendar.current.startOfDay(for: date()), value: value))
  }
  
  fileprivate mutating func save() {
    purge()
    if let data = try? JSONEncoder().encode(records) {
      storage.value = data
    }
  }
}

extension P3ATimedStorage.Record: Equatable where Value: Equatable {}
extension P3ATimedStorage.Record: Hashable where Value: Hashable {}

extension P3ATimedStorage where Value: Comparable {
  /// Gets the smallest value out of all the records stored
  public var minimumValue: Value? {
    records.map(\.value).min()
  }
  /// Gets the largest value out of all the records stored
  public var maximumValue: Value? {
    records.map(\.value).max()
  }
  /// Appends a value if the value given is larger than any value recorded today
  public mutating func replaceTodaysRecordsIfLargest(value: Value) {
    let today = Calendar.current.startOfDay(for: date())
    let todaysRecords = records.filter({ Calendar.current.startOfDay(for: $0.date) == today })
    guard let maxValue = todaysRecords.map(\.value).max() else {
      append(value: value)
      return
    }
    if value > maxValue {
      records.removeAll(where: { record in todaysRecords.contains(where: { $0.id == record.id })})
      append(value: value)
    }
  }
}

extension P3ATimedStorage where Value: AdditiveArithmetic & Comparable {
  /// Returns the sum of all the records values or 0 if no values are recorded
  public var combinedValue: Value {
    let values = records.map(\.value)
    if values.isEmpty { return .zero }
    return max(.zero, values.reduce(.zero, { $0 + $1 }))
  }
  /// Groups and combines the values of each day's values recorded then returns the largest value in that set
  public var maximumDaysCombinedValue: Value? {
    return Dictionary(grouping: records, by: { Calendar.current.startOfDay(for: $0.date) })
      .mapValues({ return $0.reduce(.zero, { $0 + $1.value }) })
      .values
      .max()
  }
  /// Adds the given value to a record with the same date in the current data set.
  ///
  /// If no record exists with the given date and the date is valid within the lifetime, then a new record
  /// will be added with the value provided.
  ///
  /// Use this when you want to add a value to a specific day's record instead of writing multiple values
  /// that will contain the same date
  public mutating func add(value: Value, to date: Date) {
    defer { save() }
    let date = Calendar.current.startOfDay(for: date)
    if let index = records.firstIndex(where: { $0.date == date }) {
      records[index].value += value
    } else if date > cutoffTime {
      records.append(.init(id: .init(), date: date, value: value))
    }
  }
}
