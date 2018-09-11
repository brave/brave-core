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
        func migrate<T>(key: String, to option: Preferences.Option<T>) {
            self.migrate(keyPrefix: keyPrefix, key: key, to: option)
        }
        
        // General
        // TODO: Search engine migration
        migrate(key: "saveLogins", to: Preferences.General.saveLogins)
        migrate(key: "blockPopups", to: Preferences.General.blockPopups)
        migrate(key: "kPrefKeyTabsBarShowPolicy", to: Preferences.General.tabBarVisibility)
        migrate(key: "thirdPartyPasswordShortcutEnabled", to: Preferences.General.passwordManagerShortcutBehavior)
        
        // Privacy
        migrate(key: "braveAcceptCookiesPref", to: Preferences.Privacy.cookieAcceptPolicy)
        migrate(key: "privateBrowsingAlwaysOn", to: Preferences.Privacy.privateBrowsingOnly)
        migrate(key: "clearprivatedata.toggles", to: Preferences.Privacy.clearPrivateDataToggles)
        
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
        
        // BraveShared
        migrateBraveShared(keyPrefix: keyPrefix)
        
        Preferences.Migration.completed.value = true
    }
}
