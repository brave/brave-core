// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared

extension Preferences {
  public final class DAU {
    public static let lastLaunchInfo = Option<[Int?]?>(key: "dau.last-launch-info", default: nil)
    public static let weekOfInstallation = Option<String?>(key: "dau.week-of-installation", default: nil)
    // On old codebase we checked existence of `dau_stat` to determine whether it's first server ping.
    // We need to translate that to use the new `firstPingParam` preference.
    static let firstPingParam: Option<Bool> =
      Option<Bool>(key: "dau.first-ping", default: Preferences.DAU.lastLaunchInfo.value == nil)
    /// Date of installation, this preference is removed after 14 days of usage.
    public static let installationDate = Option<Date?>(key: "dau.installation-date", default: nil)
    /// The app launch date after retention
    public static let appRetentionLaunchDate = Option<Date?>(key: "dau.app-retention-launch-date", default: nil)
  }
  
  public final class URP {
    public static let nextCheckDate = Option<TimeInterval?>(key: "urp.next-check-date", default: nil)
    public static let retryCountdown = Option<Int?>(key: "urp.retry-countdown", default: nil)
    public static let downloadId = Option<String?>(key: "urp.referral.download-id", default: nil)
    public static let referralCode = Option<String?>(key: "urp.referral.code", default: nil)
    public static let referralCodeDeleteDate = Option<TimeInterval?>(key: "urp.referral.delete-date", default: nil)
    /// Whether the ref code lookup has still yet to occur
    public static let referralLookupOutstanding = Option<Bool?>(key: "urp.referral.lookkup-completed", default: nil)
  }

}
