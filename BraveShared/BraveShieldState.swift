import Foundation
import Deferred
import Shared
import SwiftyJSON

// These override the setting in the prefs
public struct BraveShieldState {

    public static func set(forDomain domain: String, state: (BraveShieldState.Shield, Bool?)) {
        BraveShieldState.setInMemoryforDomain(domain, setState: state)

        // BRAVE TODO:
//        if PrivateBrowsing.singleton.isOn {
//            return
//        }
//
//        let context = DataController.workerThreadContext
//        context.perform {
//            Domain.setBraveShield(forDomain: domain, state: state, context: context)
//        }
    }

    public enum Shield : String {
        case AllOff = "all_off"
        case AdblockAndTp = "adblock_and_tp"
        case HTTPSE = "httpse"
        case SafeBrowsing = "safebrowsing"
        case FpProtection = "fp_protection"
        case NoScript = "noscript"
    }

    fileprivate var state = [Shield:Bool]()

    typealias DomainKey = String
    static var perNormalizedDomain = [DomainKey: BraveShieldState]()

    public static func setInMemoryforDomain(_ domain: String, setState state:(BraveShieldState.Shield, Bool?)) {
        var shields = perNormalizedDomain[domain]
        if shields == nil {
            if state.1 == nil {
                return
            }
            shields = BraveShieldState()
        }

        shields!.setState(state.0, on: state.1)
        perNormalizedDomain[domain] = shields!
    }

    public static func getStateForDomain(_ domain: String) -> BraveShieldState? {
        return perNormalizedDomain[domain]
    }

    public init(jsonStateFromDbRow: String) {
        let js = JSON(parseJSON: jsonStateFromDbRow)
        for (k,v) in (js.dictionary ?? [:]) {
            if let key = Shield(rawValue: k) {
                setState(key, on: v.bool)
            } else {
                assert(false, "db has bad brave shield state")
            }
        }
    }

    public init() {
    }

    public init(orig: BraveShieldState) {
        self.state = orig.state // Dict value type is copied
    }

    func toJsonString() -> String? {
        var _state = [String: Bool]()
        for (k, v) in state {
            _state[k.rawValue] = v
        }
        return JSON(_state).rawString()
    }

    mutating func setState(_ key: Shield, on: Bool?) {
        if let on = on {
            state[key] = on
        } else {
            state.removeValue(forKey: key)
        }
    }

    /// Gets whether or not the a site-specific shield override is enabled, or returns nil if it hasn't been set
    public func isShieldOverrideEnabled(_ shield: Shield) -> Bool? {
        return state[shield]
    }

    mutating func setStateFromPerPageShield(_ pageState: BraveShieldState?) {
        // BRAVE TODO:
//        setState(.NoScript, on: pageState?.isOnScriptBlocking() ?? (BraveApp.getPrefs()?.boolForKey(kPrefKeyNoScriptOn) ?? false))
//        setState(.AdblockAndTp, on: pageState?.isOnAdBlockAndTp() ?? AdBlocker.singleton.isNSPrefEnabled)
//        setState(.SafeBrowsing, on: pageState?.isOnSafeBrowsing() ?? SafeBrowsing.singleton.isNSPrefEnabled)
//        setState(.HTTPSE, on: pageState?.isOnHTTPSE() ?? HttpsEverywhere.singleton.isNSPrefEnabled)
//        setState(.FpProtection, on: pageState?.isOnFingerprintProtection() ?? (BraveApp.getPrefs()?.boolForKey(kPrefKeyFingerprintProtection) ?? false))
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
