//
//  Migration.swift
//  Client
//
//  Created by Kyle Hickinson on 2018-07-03.
//  Copyright © 2018 Mozilla. All rights reserved.
//

import Foundation
import Shared
import SwiftKeychainWrapper

extension Preferences {
    /// Migration preferences
    final class Migration {
        static let completed = Option<Bool>(key: "migration.completed", default: false)
    }
    
    /// Migrate the users preferences from prior versions of the app (<2.0)
    class func migrate(from profile: Profile) {
        if Preferences.Migration.completed.value {
            return
        }
        
        // Grab the user defaults that Prefs saves too and the key prefix all objects inside it are saved under
        let userDefaults = UserDefaults(suiteName: AppInfo.sharedContainerIdentifier)
        let keyPrefix = profile.prefs.getBranchPrefix()
        
        /// Migrate a given key from `Prefs` into a specific option
        func migrate<T>(key: String, to option: Preferences.Option<T>) {
            let profileKey = "\(keyPrefix)\(key)"
            if let value = userDefaults?.object(forKey: profileKey) as? T {
                option.value = value
                userDefaults?.removeObject(forKey: profileKey)
            }
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
        
        // Support
        migrate(key: "userallowstelemetry", to: Preferences.Support.sendsCrashReportsAndMetrics)
        
        // Popups
        migrate(key: "popupForDDG", to: Preferences.Popups.duckDuckGoPrivateSearch)
        
        Preferences.Migration.completed.value = true
    }
}
