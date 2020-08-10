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
}
