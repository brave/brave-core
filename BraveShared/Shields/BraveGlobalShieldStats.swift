/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import SwiftyJSON

open class BraveGlobalShieldStats {
    public static let shared = BraveGlobalShieldStats()
    public static let didUpdateNotification = "BraveGlobalShieldStatsDidUpdate"

    public var adblock: Int = 0 {
        didSet {
            Preferences.BlockStats.adsCount.value = adblock
            postUpdateNotification()
        }
    }

    public var trackingProtection: Int = 0 {
        didSet {
            Preferences.BlockStats.trackersCount.value = trackingProtection
            postUpdateNotification()
        }
    }

    public var httpse: Int = 0 {
        didSet {
            Preferences.BlockStats.httpsUpgradeCount.value = httpse
            postUpdateNotification()
        }
    }
    
    public var scripts: Int = 0 {
        didSet {
            Preferences.BlockStats.scriptsCount.value = scripts
            postUpdateNotification()
        }
    }
    
    public var images: Int = 0 {
        didSet {
            Preferences.BlockStats.imagesCount.value = images
            postUpdateNotification()
        }
    }
    
    public var safeBrowsing: Int = 0 {
        didSet {
            Preferences.BlockStats.phishingCount.value = safeBrowsing
            postUpdateNotification()
        }
    }
    
    public var fpProtection: Int = 0 {
        didSet {
            Preferences.BlockStats.fingerprintingCount.value = fpProtection
            postUpdateNotification()
        }
    }
    
    private func postUpdateNotification() {
        NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.didUpdateNotification), object: nil)
    }

    fileprivate init() {
        adblock = Preferences.BlockStats.adsCount.value
        trackingProtection = Preferences.BlockStats.trackersCount.value
        httpse = Preferences.BlockStats.httpsUpgradeCount.value
        images = Preferences.BlockStats.imagesCount.value
        scripts = Preferences.BlockStats.scriptsCount.value
        fpProtection = Preferences.BlockStats.fingerprintingCount.value
        safeBrowsing = Preferences.BlockStats.phishingCount.value
    }
    
    fileprivate let millisecondsPerItem: Int = 50
    fileprivate let bytesPerItem = 30485
    
    public var timeSaved: String {
        get {
            let estimatedMillisecondsSaved = (adblock + trackingProtection) * millisecondsPerItem
            let hours = estimatedMillisecondsSaved < 1000 * 60 * 60 * 24
            let minutes = estimatedMillisecondsSaved < 1000 * 60 * 60
            let seconds = estimatedMillisecondsSaved < 1000 * 60
            var counter: Double = 0
            var text = ""
            
            if seconds {
                counter = ceil(Double(estimatedMillisecondsSaved / 1000))
                text = Strings.shieldsTimeStatsSeconds
            } else if minutes {
                counter = ceil(Double(estimatedMillisecondsSaved / 1000 / 60))
                text = Strings.shieldsTimeStatsMinutes
            } else if hours {
                counter = ceil(Double(estimatedMillisecondsSaved / 1000 / 60 / 60))
                text = Strings.shieldsTimeStatsHour
            } else {
                counter = ceil(Double(estimatedMillisecondsSaved / 1000 / 60 / 60 / 24))
                text = Strings.shieldsTimeStatsDays
            }
            
            if let counterLocaleStr = Int(counter).decimalFormattedString {
                return counterLocaleStr + text
            } else {
                return "0" + Strings.shieldsTimeStatsSeconds     // If decimalFormattedString returns nil, default to "0s"
            }
        }
    }
    
    public var dataSaved: String {
        var estimatedDataSavedInBytes = (BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection) * bytesPerItem
        
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

private extension Int {
    var decimalFormattedString: String? {
        let numberFormatter = NumberFormatter()
        numberFormatter.numberStyle = NumberFormatter.Style.decimal
        numberFormatter.locale = NSLocale.current
        return numberFormatter.string(from: self as NSNumber)
    }
}
