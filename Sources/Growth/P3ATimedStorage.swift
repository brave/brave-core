// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

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
    calendar.date(
      byAdding: .day,
      value: -lifetimeInDays,
      to: calendar.startOfDay(for: date())
    ) ?? Date()
  }
  
  fileprivate let calendar: Calendar = .init(identifier: .gregorian)
  
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
    let today = calendar.startOfDay(for: date())
    let todaysRecords = records.filter({ calendar.startOfDay(for: $0.date) == today })
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
  public var maximumDaysCombinedValue: Value {
    return Dictionary(grouping: records, by: { calendar.startOfDay(for: $0.date) })
      .mapValues({ return $0.reduce(.zero, { $0 + $1.value }) })
      .values
      .max() ?? .zero
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
    let date = calendar.startOfDay(for: date)
    if let index = records.firstIndex(where: { $0.date == date }) {
      records[index].value += value
    } else if date > cutoffTime {
      records.append(.init(id: .init(), date: date, value: value))
    }
  }
}
