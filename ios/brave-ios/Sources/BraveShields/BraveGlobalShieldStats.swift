// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import Strings

/// Shield stats for a profile.
///
/// Ads, trackers and bandwidth saved are stored in the profile's Chromium prefs (shared with
/// desktop & Android)
@Observable
public class BraveGlobalShieldStats {
  /// Bandwidth saved is estimated per blocked item
  public static let averageBytesSavedPerItem = 30485

  /// Bandwidth saved in `Preferences.BlockStats`. Estimated from the blocked counts if never stored.
  public static var storedBandwidthSavedBytes: Int {
    Preferences.BlockStats.bandwidthSavedBytes.value
      ?? (Preferences.BlockStats.adsCount.value + Preferences.BlockStats.trackersCount.value)
      * averageBytesSavedPerItem
  }

  @ObservationIgnored private let prefs: any PrefService
  @ObservationIgnored private var prefsChangeRegistrar: PrefChangeRegistrar?
  @ObservationIgnored private var isUpdatingFromPrefs = false

  // Number of ads blocked
  public var adblock: Int = 0 {
    didSet {
      guard !isUpdatingFromPrefs, adblock != oldValue else { return }
      // Pref observers fire synchronously and reassign `adblock` from prefs, so capture the
      // values before writing.
      let newCount = UInt64(max(adblock, 0))
      let addedBandwidth = UInt64(max(adblock - oldValue, 0) * Self.averageBytesSavedPerItem)
      let bandwidth = prefs.uint64(forPath: kBandwidthSavedBytesPrefName) + addedBandwidth
      prefs.set(newCount, forPath: kAdsBlockedPrefName)
      prefs.set(bandwidth, forPath: kBandwidthSavedBytesPrefName)
    }
  }

  /// Number of trackers blocked. No longer incremented (all platforms),
  /// legacy counts only.
  private var trackingProtection: Int = 0

  // Total number of ads and trackers blocked
  public var adsAndTrackersBlocked: Int {
    adblock + trackingProtection
  }

  /// Estimated bandwidth saved in bytes
  public private(set) var bandwidthSavedBytes: Int = 0

  /// Number of scripts blocked
  public var scripts: Int = 0 {
    didSet {
      Preferences.BlockStats.scriptsCount.value = scripts
    }
  }

  /// Number of images blocked
  public var images: Int = 0 {
    didSet {
      Preferences.BlockStats.imagesCount.value = images
    }
  }

  public init(prefs: any PrefService) {
    self.prefs = prefs
    images = Preferences.BlockStats.imagesCount.value
    scripts = Preferences.BlockStats.scriptsCount.value

    // Observe changes core-side and keep `Preferences.BlockStats` in sync
    let registrar = PrefChangeRegistrar(prefService: prefs)
    for path in [kAdsBlockedPrefName, kTrackersBlockedPrefName, kBandwidthSavedBytesPrefName] {
      registrar.addObserver(forPath: path) { [weak self] _ in
        self?.updateFromPrefs()
      }
    }
    prefsChangeRegistrar = registrar
    updateFromPrefs()
  }

  private func updateFromPrefs() {
    isUpdatingFromPrefs = true
    defer { isUpdatingFromPrefs = false }
    adblock = Int(clamping: prefs.uint64(forPath: kAdsBlockedPrefName))
    trackingProtection = Int(clamping: prefs.uint64(forPath: kTrackersBlockedPrefName))
    bandwidthSavedBytes = Int(clamping: prefs.uint64(forPath: kBandwidthSavedBytesPrefName))
    // keep `Preferences.BlockStats` so extensions (without Profile access) can read stats
    Preferences.BlockStats.adsCount.value = adblock
    Preferences.BlockStats.trackersCount.value = trackingProtection
    Preferences.BlockStats.bandwidthSavedBytes.value = bandwidthSavedBytes
  }

  // Formatted time saved from blocking
  public var timeSaved: String {
    Self.timeSaved(blockedCount: adblock + trackingProtection)
  }

  // Formatted data saved from blocking
  public var dataSaved: String {
    Self.dataSaved(bytes: bandwidthSavedBytes)
  }

  /// Formats the given `blockedCount` to time saved string
  public static func timeSaved(blockedCount: Int) -> String {
    let millisecondsPerItem: Int = 50
    let estimatedMillisecondsSaved = blockedCount * millisecondsPerItem
    let hours = estimatedMillisecondsSaved < 1000 * 60 * 60 * 24
    let minutes = estimatedMillisecondsSaved < 1000 * 60 * 60
    let seconds = estimatedMillisecondsSaved < 1000 * 60
    var counter: Double = 0
    var text = ""

    if seconds {
      counter = ceil(Double(estimatedMillisecondsSaved / 1000))
      text = Strings.Shields.shieldsTimeStatsSeconds
    } else if minutes {
      counter = ceil(Double(estimatedMillisecondsSaved / 1000 / 60))
      text = Strings.Shields.shieldsTimeStatsMinutes
    } else if hours {
      counter = ceil(Double(estimatedMillisecondsSaved / 1000 / 60 / 60))
      text = Strings.Shields.shieldsTimeStatsHour
    } else {
      counter = ceil(Double(estimatedMillisecondsSaved / 1000 / 60 / 60 / 24))
      text = Strings.Shields.shieldsTimeStatsDays
    }

    if let counterLocaleStr = Int(counter).decimalFormattedString {
      return counterLocaleStr + text
    } else {
      // If decimalFormattedString returns nil, default to "0s"
      return "0" + Strings.Shields.shieldsTimeStatsSeconds
    }
  }

  /// Formats the given `bytes` to data saved string
  public static func dataSaved(bytes: Int) -> String {
    var estimatedDataSavedInBytes = bytes

    if estimatedDataSavedInBytes <= 0 { return "0" }
    let _1MB = 1000 * 1000

    // Byte formatted megabytes value can be too long to display nicely(#3274).
    // As a workaround we cut fraction value from megabytes by rounding it down.
    if estimatedDataSavedInBytes > _1MB {
      estimatedDataSavedInBytes = (estimatedDataSavedInBytes / _1MB) * _1MB
    }

    let formatter = ByteCountFormatter()
    formatter.allowsNonnumericFormatting = false
    // Skip bytes, because it displays as `bytes` instead of `B` which is too long for our shield stat.
    // This is for extra safety only, there's not many sub 1000 bytes tracker scripts in the wild.
    formatter.allowedUnits = [.useKB, .useMB, .useGB, .useTB]

    return formatter.string(fromByteCount: Int64(estimatedDataSavedInBytes))
  }
}

extension Int {
  fileprivate var decimalFormattedString: String? {
    let numberFormatter = NumberFormatter()
    numberFormatter.numberStyle = NumberFormatter.Style.decimal
    numberFormatter.locale = NSLocale.current
    return numberFormatter.string(from: self as NSNumber)
  }
}
