// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

/// Monitors how long the browser is foregrounded to answer the `Brave.Uptime.BrowserOpenTime` P3A question
public class UptimeMonitor {
  private var timer: Timer?

  private(set) static var usageInterval: TimeInterval = 15
  private(set) static var now: () -> Date = { .now }
  private(set) static var calendar: Calendar = .current

  public init() {
    if Preferences.UptimeMonitor.startTime.value == nil {
      // If today is the first time monitoring uptime, set the frame start time to now.
      resetPrefs()
    }
    recordP3A()
  }

  deinit {
    timer?.invalidate()
  }

  // For testing
  var didRecordP3A: ((_ durationInMinutes: Int) -> Void)?

  public var isMonitoring: Bool {
    timer != nil
  }

  /// Begins a timer to monitor uptime
  public func beginMonitoring() {
    if isMonitoring {
      return
    }
    timer = Timer.scheduledTimer(
      withTimeInterval: Self.usageInterval,
      repeats: true,
      block: { [weak self] _ in
        guard let self else { return }
        Preferences.UptimeMonitor.uptimeSum.value += Self.usageInterval
        self.recordP3A()
      }
    )
  }
  /// Pauses the timer to monitor uptime
  public func pauseMonitoring() {
    timer?.invalidate()
    timer = nil
  }

  private func recordP3A() {
    guard let startTime = Preferences.UptimeMonitor.startTime.value,
      !Self.calendar.isDate(startTime, inSameDayAs: Self.now())
    else {
      // Do not report, since 1 day has not passed.
      return
    }
    let buckets: [Bucket] = [
      .r(0...30),
      .r(31...60),
      .r(61...120),  // 1-2 hours
      .r(121...180),  // 2-3 hours
      .r(181...300),  // 3-5 hours
      .r(301...420),  // 5-7 hours
      .r(421...600),  // 7-10 hours
      .r(601...),  // 10+ hours
    ]
    let durationInMinutes = Int(Preferences.UptimeMonitor.uptimeSum.value / 60.0)
    UmaHistogramRecordValueToBucket(
      "Brave.Uptime.BrowserOpenTime.2",
      buckets: buckets,
      value: durationInMinutes
    )
    resetPrefs()
    didRecordP3A?(durationInMinutes)
  }

  private func resetPrefs() {
    Preferences.UptimeMonitor.startTime.value = Self.now()
    Preferences.UptimeMonitor.uptimeSum.value = 0
  }

  static func setUsageIntervalForTesting(_ usageInterval: TimeInterval) {
    Self.usageInterval = usageInterval
  }

  static func setNowForTesting(_ now: @escaping () -> Date) {
    Self.now = now
  }

  static func setCalendarForTesting(_ calendar: Calendar) {
    Self.calendar = calendar
  }
}
