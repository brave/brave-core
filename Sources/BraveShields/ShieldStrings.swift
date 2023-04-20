// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Strings

extension Strings {
  public struct Shields {
    public static let shieldsAdStats = NSLocalizedString("AdsrBlocked", bundle: .module, value: "Ads \nBlocked", comment: "Shields Ads Stat")
    public static let shieldsAdAndTrackerStats = NSLocalizedString("AdsAndTrackersrBlocked", bundle: .module, value: "Trackers & ads blocked", comment: "Shields Ads Stat")
    public static let shieldsTrackerStats = NSLocalizedString("TrackersrBlocked", bundle: .module, value: "Trackers \nBlocked", comment: "Shields Trackers Stat")
    public static let dataSavedStat = NSLocalizedString("DataSavedStat", bundle: .module, value: "Est. Data \nSaved", comment: "Data Saved Shield Stat")
    public static let shieldsTimeStats = NSLocalizedString("EstTimerSaved", bundle: .module, value: "Est. Time \nSaved", comment: "Shields Time Saved Stat")
    public static let shieldsTimeStatsHour = NSLocalizedString("ShieldsTimeStatsHour", bundle: .module, value: "h", comment: "Time Saved Hours")
    public static let shieldsTimeStatsMinutes = NSLocalizedString("ShieldsTimeStatsMinutes", bundle: .module, value: "min", comment: "Time Saved Minutes")
    public static let shieldsTimeStatsSeconds = NSLocalizedString("ShieldsTimeStatsSeconds", bundle: .module, value: "s", comment: "Time Saved Seconds")
    public static let shieldsTimeStatsDays = NSLocalizedString("ShieldsTimeStatsDays", bundle: .module, value: "d", comment: "Time Saved Days")
  }
}
