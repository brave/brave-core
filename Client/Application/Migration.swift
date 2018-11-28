/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import SwiftKeychainWrapper

private let log = Logger.browserLogger

extension Preferences {
    /// Migration preferences
    final class Migration {
        static let completed = Option<Bool>(key: "migration.completed", default: false)
    }
    
    /// Migrate the users preferences from prior versions of the app (<2.0)
    class func migratePreferences(keyPrefix: String) {
        if Preferences.Migration.completed.value {
            return
        }
        
        // Grab the user defaults that Prefs saves too and the key prefix all objects inside it are saved under
        let userDefaults = UserDefaults(suiteName: AppInfo.sharedContainerIdentifier)
        
        /// Wrapper around BraveShared migrate, to automate prefix injection
        func migrate<T>(key: String, to option: Preferences.Option<T>, transform: ((T) -> T)? = nil) {
            self.migrate(keyPrefix: keyPrefix, key: key, to: option, transform: transform)
        }
        
        // General
        migrate(key: "saveLogins", to: Preferences.General.saveLogins)
        migrate(key: "blockPopups", to: Preferences.General.blockPopups)
        migrate(key: "kPrefKeyTabsBarShowPolicy", to: Preferences.General.tabBarVisibility)
        migrate(key: "NightModeStatus", to: Preferences.General.nightMode)
        
        // Search
        migrate(key: "search.orderedEngineNames", to: Preferences.Search.orderedEngines)
        migrate(key: "search.disabledEngineNames", to: Preferences.Search.disabledEngines)
        migrate(key: "search.suggestions.show", to: Preferences.Search.showSuggestions)
        migrate(key: "search.suggestions.showOptIn", to: Preferences.Search.shouldShowSuggestionsOptIn)
        migrate(key: "search.default.name", to: Preferences.Search.defaultEngineName)
        migrate(key: "search.defaultprivate.name", to: Preferences.Search.defaultPrivateEngineName)
        
        // Privacy
        let legacyCookieMap: [HTTPCookie.AcceptPolicy] = [.onlyFromMainDocumentDomain, .never, .always]
        migrate(key: "braveAcceptCookiesPref", to: Preferences.Privacy.cookieAcceptPolicy, transform: { value in
            // The value stored in 1.6 is actually mapped differently (optionIds -> HTTPCookie.AcceptPolicy) rather
            // than just use the rawValue of said AcceptPolicy like in 1.7. The mapping is shown in `legacyCookieMap`
            if value < legacyCookieMap.count {
                return legacyCookieMap[Int(value)].rawValue
            }
            // If for some odd reason we have malformed data, just set the default value
            return Preferences.Privacy.cookieAcceptPolicy.defaultValue
        })
        
        migrate(key: "privateBrowsingAlwaysOn", to: Preferences.Privacy.privateBrowsingOnly)
        migrate(key: "clearprivatedata.toggles", to: Preferences.Privacy.clearPrivateDataToggles)
        
        // Make sure to unlock all directories that may have been locked in 1.6 private mode
        let baseDir = NSSearchPathForDirectoriesInDomains(.libraryDirectory, .userDomainMask, true)[0]
        [baseDir + "/WebKit", baseDir + "/Caches"].forEach {
            do {
                try FileManager.default.setAttributes([.posixPermissions: NSNumber(value: 0o755 as Int16)], ofItemAtPath: $0)
            } catch {
                log.error("Failed setting the directory attributes for \($0)")
            }
        }
        
        // Security
        NSKeyedUnarchiver.setClass(AuthenticationKeychainInfo.self, forClassName: "AuthenticationKeychainInfo")
        if let pinLockInfo = KeychainWrapper.standard.object(forKey: "pinLockInfo") as? AuthenticationKeychainInfo {
            KeychainWrapper.sharedAppContainerKeychain.setAuthenticationInfo(pinLockInfo)
        }
        
        // Shields
        migrate(key: "braveBlockAdsAndTracking", to: Preferences.Shields.blockAdsAndTracking)
        migrate(key: "braveHttpsEverywhere", to: Preferences.Shields.httpsEverywhere)
        migrate(key: "braveSafeBrowsing", to: Preferences.Shields.blockPhishingAndMalware)
        migrate(key: "noscript_on", to: Preferences.Shields.blockScripts)
        migrate(key: "fingerprintprotection_on", to: Preferences.Shields.fingerprintingProtection)
        migrate(key: "braveAdblockUseRegional", to: Preferences.Shields.useRegionAdBlock)
        
        // Popups
        migrate(key: "popupForDDG", to: Preferences.Popups.duckDuckGoPrivateSearch)
        migrate(key: "popupForBrowserLock", to: Preferences.Popups.browserLock)
        
        // BraveShared
        migrateBraveShared(keyPrefix: keyPrefix)
        
        Preferences.Migration.completed.value = true
    }
}
