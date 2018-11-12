/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import WebKit
import Shared
import Deferred
import Data
import BraveShared

private let log = Logger.browserLogger

// Rename to BlockList
class BlocklistName: Equatable, Hashable {
    static let ad = BlocklistName(filename: "block-ads")
    static let tracker = BlocklistName(filename: "block-trackers")
    static let https = BlocklistName(filename: "upgrade-http")
    static let script = BlocklistName(filename: "block-scripts")
    static let image = BlocklistName(filename: "block-images")

    static var allLists: Set<BlocklistName> { return [.ad, .tracker, .https, .script, .image] }
    
    let filename: String
    var rule: WKContentRuleList?
    
    init(filename: String) {
        self.filename = filename
    }
    
    static func blocklists(forDomain domain: Domain) -> (on: Set<BlocklistName>, off: Set<BlocklistName>) {
        if domain.shield_allOff == 1 {
            return ([], allLists)
        }
        
        /// Returns true if a certain Blocklist, should be included for `Domain`
        func include(domainSetting domain: NSNumber?, globalValue global: Preferences.Option<Bool>) -> Bool {
            return domain == 1 || (domain == nil && global.value)
        }
        
        var onList = Set<BlocklistName>()
        
        if include(domainSetting: domain.shield_adblockAndTp, globalValue: Preferences.Shields.blockAdsAndTracking) {
            onList.formUnion([.ad, .tracker])
        }
        
        if include(domainSetting: domain.shield_noScript, globalValue: Preferences.Shields.blockScripts) {
            onList.insert(.script)
        }
        
        // TODO #159: Setup image shield
        // TODO #269: Setup HTTPS shield
        
        return (onList, allLists.subtracting(onList))
    }

    private func compile(ruleStore: WKContentRuleListStore) -> Deferred<Void> {
        let compilerDeferred = Deferred<Void>()
        
        let ruleExists = Deferred<Bool>()
        ruleStore.lookUpContentRuleList(forIdentifier: self.filename) { rule, error in
            self.rule = rule
            ruleExists.fill(self.rule != nil)
        }
        
        ruleExists.upon { exists in
            if exists {
                compilerDeferred.fill(())
                return
            }
            
            BlocklistName.loadJsonFromBundle(forResource: self.filename) { jsonString in
                ruleStore.compileContentRuleList(forIdentifier: self.filename, encodedContentRuleList: jsonString) { rule, error in
                    if let error = error {
                        // TODO #382: Potential telemetry location
                        log.error("Content blocker '\(self.filename)' errored: \(error.localizedDescription)")
                        assert(false)
                    }
                    assert(rule != nil)
                    
                    self.rule = rule
                    compilerDeferred.fill(())
                }
            }
        }
        return compilerDeferred
    }
    
    static func compileAll(ruleStore: WKContentRuleListStore) -> Deferred<Void> {
        let allCompiledDeferred = Deferred<Void>()
        let allOfThem = BlocklistName.allLists.map {
            $0.compile(ruleStore: ruleStore)
        }
        
        all(allOfThem).upon { _ in
            allCompiledDeferred.fill(())
        }
        
        return allCompiledDeferred
    }
    
    private static func loadJsonFromBundle(forResource file: String, completion: @escaping (_ jsonString: String) -> Void) {
        DispatchQueue.global().async {
            guard let path = Bundle.main.path(forResource: file, ofType: "json"),
                let source = try? String(contentsOfFile: path, encoding: .utf8) else {
                    assert(false)
                    return
            }
            
            DispatchQueue.main.async {
                completion(source)
            }
        }
    }
    
    public static func == (lhs: BlocklistName, rhs: BlocklistName) -> Bool {
        return lhs.filename == rhs.filename
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(filename)
    }
}

@available(iOS 11.0, *)
enum BlockerStatus: String {
    case Disabled
    case NoBlockedURLs // When TP is enabled but nothing is being blocked
    case Whitelisted
    case Blocking
}

struct ContentBlockingConfig {
    struct Prefs {
        static let NormalBrowsingEnabledKey = "prefkey.trackingprotection.normalbrowsing"
        static let PrivateBrowsingEnabledKey = "prefkey.trackingprotection.privatebrowsing"
    }

    struct Defaults {
        static let NormalBrowsing = true
        static let PrivateBrowsing = true
    }
}

struct NoImageModeDefaults {
    static let Script = "[{'trigger':{'url-filter':'.*','resource-type':['image']},'action':{'type':'block'}}]".replacingOccurrences(of: "'", with: "\"")
    static let ScriptName = "images"
}

enum BlockingStrength: String {
    case basic
    case strict

    static let allOptions: [BlockingStrength] = [.basic, .strict]
}

@available(iOS 11.0, *)
class ContentBlockerHelper {

    static let ruleStore: WKContentRuleListStore = WKContentRuleListStore.default()
    weak var tab: Tab?
    private(set) var userPrefs: Prefs?
    
    static func compileLists() -> Deferred<()> {
        return BlocklistName.compileAll(ruleStore: ruleStore)
    }

    var isUserEnabled: Bool? {
        didSet {
            setupTabTrackingProtection()
            guard let tab = tab else { return }
            TabEvent.post(.didChangeContentBlocking, for: tab)
            tab.reload()
        }
    }

    var isEnabled: Bool {
        if let enabled = isUserEnabled {
            return enabled
        }

        guard let tab = tab else {
            return false
        }

        switch tab.type {
        case .regular:
            return isEnabledInNormalBrowsing
        case .private:
            return isEnabledInPrivateBrowsing
        }
    }

    var status: BlockerStatus {
        guard isEnabled else {
            return .Disabled
        }
        if stats.total == 0 {
            // TODO: 161, _may_ need to handle whitelisted situations here
            return .NoBlockedURLs
        } else {
            return .Blocking
        }
    }

    var stats: TPPageStats = TPPageStats() {
        didSet {
            guard let tab = self.tab else { return }
            if stats.total <= 1 {
                TabEvent.post(.didChangeContentBlocking, for: tab)
            }
        }
    }

    fileprivate var isEnabledInNormalBrowsing: Bool {
        return userPrefs?.boolForKey(ContentBlockingConfig.Prefs.NormalBrowsingEnabledKey) ?? ContentBlockingConfig.Defaults.NormalBrowsing
    }

    var isEnabledInPrivateBrowsing: Bool {
        return userPrefs?.boolForKey(ContentBlockingConfig.Prefs.PrivateBrowsingEnabledKey) ?? ContentBlockingConfig.Defaults.PrivateBrowsing
    }

    static private var blockImagesRule: WKContentRuleList?
    static var heavyInitHasRunOnce = false

    init(tab: Tab, profile: Profile) {
        self.tab = tab
        self.userPrefs = profile.prefs

        NotificationCenter.default.addObserver(self, selector: #selector(setupTabTrackingProtection), name: .ContentBlockerTabSetupRequired, object: nil)
    }

    class func prefsChanged() {
        // This class func needs to notify all the active instances of ContentBlockerHelper to update.
        NotificationCenter.default.post(name: .ContentBlockerTabSetupRequired, object: nil)
    }

    deinit {
        NotificationCenter.default.removeObserver(self)
    }

    // Function to install or remove TP for a tab
    @objc func setupTabTrackingProtection() {
        if !ContentBlockerHelper.heavyInitHasRunOnce {
            return
        }

        removeTrackingProtection()

        if !isEnabled {
            return
        }

        let rules = BlocklistName.allLists
        for list in rules {
            let name = list.filename
            ContentBlockerHelper.ruleStore.lookUpContentRuleList(forIdentifier: name) { rule, error in
                guard let rule = rule else {
                    let msg = "lookUpContentRuleList for \(name):  \(error?.localizedDescription ?? "empty rules")"
                    log.error("Content blocker error: \(msg)")
                    return
                }
                self.addToTab(contentRuleList: rule)
            }
        }
    }

    private func removeTrackingProtection() {
        guard let tab = tab else { return }
        tab.webView?.configuration.userContentController.removeAllContentRuleLists()

        if let rule = ContentBlockerHelper.blockImagesRule, tab.noImageMode {
            addToTab(contentRuleList: rule)
        }
    }

    private func addToTab(contentRuleList: WKContentRuleList) {
        tab?.webView?.configuration.userContentController.add(contentRuleList)
    }

    func noImageMode(enabled: Bool) {
        guard let rule = ContentBlockerHelper.blockImagesRule else { return }

        if enabled {
            addToTab(contentRuleList: rule)
        } else {
            tab?.webView?.configuration.userContentController.remove(rule)
        }

        // Async required here to ensure remove() call is processed.
        DispatchQueue.main.async() {
            self.tab?.webView?.evaluateJavaScript("window.__firefox__.NoImageMode.setEnabled(\(enabled))")
        }
    }

}

// MARK: Static methods to check if Tracking Protection is enabled in the user's prefs
@available(iOS 11.0, *)
extension ContentBlockerHelper {

    static func setTrackingProtectionMode(_ enabled: Bool, for prefs: Prefs, with tabManager: TabManager) {
        guard let selectedTab = tabManager.selectedTab else {
            return
        }

        let key: String

        switch selectedTab.type {
        case .regular:
            key = ContentBlockingConfig.Prefs.NormalBrowsingEnabledKey
        case .private:
            key = ContentBlockingConfig.Prefs.PrivateBrowsingEnabledKey
        }

        prefs.setBool(enabled, forKey: key)
        ContentBlockerHelper.prefsChanged()
    }

    static func isTrackingProtectionActive(tabManager: TabManager) -> Bool {
        guard let selectedTab = tabManager.selectedTab, let blocker = selectedTab.contentBlocker as? ContentBlockerHelper else {
            return false
        }

        switch selectedTab.type {
        case .regular:
            return blocker.isEnabledInNormalBrowsing
        case .private:
            return blocker.isEnabledInPrivateBrowsing
        }
    }

    static func toggleTrackingProtectionMode(for prefs: Prefs, tabManager: TabManager) {
        let isEnabled = ContentBlockerHelper.isTrackingProtectionActive(tabManager: tabManager)
        setTrackingProtectionMode(!isEnabled, for: prefs, with: tabManager)
    }
}

