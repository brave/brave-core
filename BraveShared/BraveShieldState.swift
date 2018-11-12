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
    static let singleton = BraveGlobalShieldStats()
    static let DidUpdateNotification = "BraveGlobalShieldStatsDidUpdate"
    
    // BRAVE TODO:
//    fileprivate let prefs = BraveApp.getPrefs()
    
    var adblock: Int32 = 0 {
        didSet {
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }

    var trackingProtection: Int32 = 0 {
        didSet {
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }

    var httpse: Int32 = 0 {
        didSet {
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }
    
    var safeBrowsing: Int32 = 0 {
        didSet {
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }
    
    var fpProtection: Int32 = 0 {
        didSet {
            NotificationCenter.default.post(name: Notification.Name(rawValue: BraveGlobalShieldStats.DidUpdateNotification), object: nil)
        }
    }

    enum Shield: String {
        case Adblock = "adblock"
        case TrackingProtection = "tracking_protection"
        case HTTPSE = "httpse"
        case SafeBrowsing = "safebrowsing"
        case FpProtection = "fp_protection"
    }
    
    fileprivate init() {
        // BRAVE TODO:
//        let userDefaults = UserDefaults.standard
//        if (prefs?.intForKey(Shield.Adblock.rawValue) ?? 0) == 0 {
//            prefs?.setInt(Int32(userDefaults.integer(forKey: Shield.Adblock.rawValue)), forKey: Shield.Adblock.rawValue)
//            prefs?.setInt(Int32(userDefaults.integer(forKey: Shield.TrackingProtection.rawValue)), forKey: Shield.TrackingProtection.rawValue)
//            prefs?.setInt(Int32(userDefaults.integer(forKey: Shield.HTTPSE.rawValue)), forKey: Shield.HTTPSE.rawValue)
//            prefs?.setInt(Int32(userDefaults.integer(forKey: Shield.SafeBrowsing.rawValue)), forKey: Shield.SafeBrowsing.rawValue)
//            prefs?.setInt(Int32(userDefaults.integer(forKey: Shield.FpProtection.rawValue)), forKey: Shield.FpProtection.rawValue)
//            prefs?.userDefaults.synchronize()
//        }
//
//        adblock += prefs?.intForKey(Shield.Adblock.rawValue) ?? 0
//        trackingProtection += prefs?.intForKey(Shield.TrackingProtection.rawValue) ?? 0
//        httpse += prefs?.intForKey(Shield.HTTPSE.rawValue) ?? 0
//        safeBrowsing += prefs?.intForKey(Shield.SafeBrowsing.rawValue) ?? 0
//        fpProtection += prefs?.intForKey(Shield.FpProtection.rawValue) ?? 0
    }

    var bgSaveTask: UIBackgroundTaskIdentifier?

    open func save() {
        if let t = bgSaveTask, t != UIBackgroundTaskInvalid {
            return
        }
        
        bgSaveTask = UIApplication.shared.beginBackgroundTask(withName: "brave-global-stats-save", expirationHandler: {
            if let task = self.bgSaveTask {
                UIApplication.shared.endBackgroundTask(task)
            }
            self.bgSaveTask = UIBackgroundTaskInvalid
        })
        
        DispatchQueue.main.async {
            // BRAVE TODO:
//            self.prefs?.setInt(self.adblock, forKey: Shield.Adblock.rawValue)
//            self.prefs?.setInt(self.trackingProtection, forKey: Shield.TrackingProtection.rawValue)
//            self.prefs?.setInt(self.httpse, forKey: Shield.HTTPSE.rawValue)
//            self.prefs?.setInt(self.safeBrowsing, forKey: Shield.SafeBrowsing.rawValue)
//            self.prefs?.setInt(self.fpProtection, forKey: Shield.FpProtection.rawValue)
//            self.prefs?.userDefaults.synchronize()

            if let task = self.bgSaveTask {
                UIApplication.shared.endBackgroundTask(task)
            }
            self.bgSaveTask = UIBackgroundTaskInvalid
        }
    }
}
