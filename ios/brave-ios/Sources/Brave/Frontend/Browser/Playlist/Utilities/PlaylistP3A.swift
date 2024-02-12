// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Growth

extension P3AFeatureUsage {
  fileprivate static let playlistUsage: Self = .init(
    name: "playlist-usage",
    histogram: "Brave.Playlist.LastUsageTime",
    returningUserHistogram: "Brave.Playlist.NewUserReturning"
  )
}

extension P3ATimedStorage where Value == Int {
  fileprivate static var playlistDaysInWeekUsedStorage: Self { .init(name: "playlist-days-in-week-usage", lifetimeInDays: 7) }
}

struct PlaylistP3A {
  static func recordHistogram() {
    P3AFeatureUsage.playlistUsage.recordHistogram()
  }
  
  static func recordUsage() {
    let usage = P3AFeatureUsage.playlistUsage
    usage.recordUsage()
    
    // // Q90. As a user that used Playlist this week, how many days did I use or add content to Brave Playlist in the past 7 days?
    var daysUsedInWeekStorage = P3ATimedStorage<Int>.playlistDaysInWeekUsedStorage
    daysUsedInWeekStorage.replaceTodaysRecordsIfLargest(value: 1)
    
    let weeklyBucket: [Bucket] = [
      .r(1...2),
      .r(3...4),
      .r(5...6),
      7
    ]
    
    UmaHistogramRecordValueToBucket("Brave.Playlist.UsageDaysInWeek", buckets: weeklyBucket, value: daysUsedInWeekStorage.combinedValue)
  }
}
