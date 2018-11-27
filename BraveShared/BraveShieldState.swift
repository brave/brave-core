/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Deferred
import Shared
import SwiftyJSON

// These override the setting in the prefs
public struct BraveShieldState {
    public enum Shield: String {
        case AllOff = "all_off"
        case AdblockAndTp = "adblock_and_tp"
        case HTTPSE = "httpse"
        case SafeBrowsing = "safebrowsing"
        case FpProtection = "fp_protection"
        case NoScript = "noscript"
    }
}

open class BraveGlobalShieldStats {
    public static let shared = BraveGlobalShieldStats()
    public static let DidUpdateNotification = "BraveGlobalShieldStatsDidUpdate"
    
    public var adblock: Int = 0 {
        didSet {
            Preferences.BlockStats.adsCount.value = adblock
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }

    public var trackingProtection: Int = 0 {
        didSet {
            Preferences.BlockStats.trackersCount.value = trackingProtection
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }

    public var httpse: Int = 0 {
        didSet {
            Preferences.BlockStats.httpsUpgradeCount.value = httpse
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }
    
    public var scripts: Int = 0 {
        didSet {
            Preferences.BlockStats.scriptsCount.value = scripts
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }
    
    public var images: Int = 0 {
        didSet {
            Preferences.BlockStats.imagesCount.value = images
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }
    
    public var safeBrowsing: Int = 0 {
        didSet {
            Preferences.BlockStats.phishingCount.value = safeBrowsing
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }
    
    public var fpProtection: Int = 0 {
        didSet {
            Preferences.BlockStats.fingerprintingCount.value = fpProtection
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
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
