// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

extension Preferences {
  public final class DAU {
    public static let lastLaunchInfo = Option<[Int]?>(key: "dau.last-launch-info", default: nil)
    public static let weekOfInstallation = Option<String?>(key: "dau.week-of-installation", default: nil)
    // On old codebase we checked existence of `dau_stat` to determine whether it's first server ping.
    // We need to translate that to use the new `firstPingParam` preference.
    static let firstPingParam: Option<Bool> =
      Option<Bool>(key: "dau.first-ping", default: Preferences.DAU.lastLaunchInfo.value == nil)
    /// Date of installation, this preference is removed after 14 days of usage.
    public static let installationDate = Option<Date?>(key: "dau.installation-date", default: nil)
    /// The app launch date after retention
    public static let appRetentionLaunchDate = Option<Date?>(key: "dau.app-retention-launch-date", default: nil)
    public static let sendUsagePing = Option<Bool>(key: "dau.send-usage-ping", default: true)
  }
  
  public final class URP {
    public static let nextCheckDate = Option<TimeInterval?>(key: "urp.next-check-date", default: nil)
    public static let retryCountdown = Option<Int?>(key: "urp.retry-countdown", default: nil)
    public static let downloadId = Option<String?>(key: "urp.referral.download-id", default: nil)
    public static let referralCode = Option<String?>(key: "urp.referral.code", default: nil)
    public static let referralCodeDeleteDate = Option<TimeInterval?>(key: "urp.referral.delete-date", default: nil)
    /// Whether the ref code lookup has still yet to occur
    public static let referralLookupOutstanding = Option<Bool?>(key: "urp.referral.lookkup-completed", default: nil)
    public static let installAttributionLookupOutstanding = Option<Bool?>(key: "install.attribution.lookup-completed", default: nil)
  }

  public final class Review {
    /// Application Launch Count (how many times the application has been launched)
    public static let launchCount = Option<Int>(key: "review.launch-count", default: 0)
    public static let braveNewsCriteriaPassed = Option<Bool>(key: "review.brave-new.criteria", default: false)
    public static let numberBookmarksAdded =  Option<Int>(key: "review.numberBookmarksAdded", default: 0)
    public static let numberPlaylistItemsAdded =  Option<Int>(key: "review.numberPlaylistItemsAdded", default: 0)
    public static let dateWalletConnectedToDapp =  Option<Date?>(key: "review.connect-dapp.wallet", default: nil)
    public static let daysInUse = Option<[Date]>(key: "review.in-use", default: [])
    /// Review Threshold (the total amount of launches needed for the next review to show up) Default Value first Threashold which will be 14
    public static let threshold = Option<Int>(key: "review.threshold", default: 14)
    public static let lastReviewDate = Option<Date?>(key: "review.last-date", default: nil)
    /// The date when the rating card in news feed is shown
    public static let newsCardShownDate = Option<Date?>(key: "review.news-card", default: nil)
  }
}
