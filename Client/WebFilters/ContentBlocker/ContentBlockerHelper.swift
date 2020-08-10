/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import WebKit
import Shared
import Data
import BraveShared

private let log = Logger.browserLogger

enum BlockerStatus: String {
    case Disabled
    case NoBlockedURLs // When TP is enabled but nothing is being blocked
    case Whitelisted
    case Blocking
}

struct ContentBlockingConfig {
    struct Prefs {
        static let normalBrowsingEnabledKey = "prefkey.trackingprotection.normalbrowsing"
        static let privateBrowsingEnabledKey = "prefkey.trackingprotection.privatebrowsing"
    }

    struct Defaults {
        static let normalBrowsing = true
        static let privateBrowsing = true
    }
}

struct NoImageModeDefaults {
    static let script = "[{'trigger':{'url-filter':'.*','resource-type':['image']},'action':{'type':'block'}}]".replacingOccurrences(of: "'", with: "\"")
    static let scriptName = "images"
}

enum BlockingStrength: String {
    case basic
    case strict

    static let allOptions: [BlockingStrength] = [.basic, .strict]
}

class ContentBlockerHelper {

    static let ruleStore: WKContentRuleListStore = WKContentRuleListStore.default()
    weak var tab: Tab?
    
    static func compileBundledLists() -> Deferred<()> {
        return BlocklistName.compileBundledRules(ruleStore: ruleStore)
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
        return isUserEnabled ?? (tab != nil)
    }

    var stats: TPPageStats = TPPageStats() {
        didSet {
            guard let tab = self.tab else { return }
            if stats.total <= 1 {
                TabEvent.post(.didChangeContentBlocking, for: tab)
            }
            statsDidChange?(stats)
        }
    }
    
    var statsDidChange: ((TPPageStats) -> Void)?

    static private var blockImagesRule: WKContentRuleList?
    static var heavyInitHasRunOnce = false

    init(tab: Tab) {
        self.tab = tab

        NotificationCenter.default.addObserver(self, selector: #selector(setupTabTrackingProtection), name: .contentBlockerTabSetupRequired, object: nil)
    }

    class func prefsChanged() {
        // This class func needs to notify all the active instances of ContentBlockerHelper to update.
        NotificationCenter.default.post(name: .contentBlockerTabSetupRequired, object: nil)
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

extension ContentBlockerHelper {

    static func setTrackingProtectionMode(_ enabled: Bool, for prefs: Prefs, with tabManager: TabManager) {
        guard let selectedTab = tabManager.selectedTab else {
            return
        }

        let key: String

        switch selectedTab.type {
        case .regular:
            key = ContentBlockingConfig.Prefs.normalBrowsingEnabledKey
        case .private:
            key = ContentBlockingConfig.Prefs.privateBrowsingEnabledKey
        }

        prefs.setBool(enabled, forKey: key)
        ContentBlockerHelper.prefsChanged()
    }

    static func isTrackingProtectionActive(tabManager: TabManager) -> Bool {
        return tabManager.selectedTab != nil
    }

    static func toggleTrackingProtectionMode(for prefs: Prefs, tabManager: TabManager) {
        let isEnabled = ContentBlockerHelper.isTrackingProtectionActive(tabManager: tabManager)
        setTrackingProtectionMode(!isEnabled, for: prefs, with: tabManager)
    }
}

