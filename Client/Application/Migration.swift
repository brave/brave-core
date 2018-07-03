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
        func migrate<T>(key: String, option: Preferences.Option<T>) {
            if let value = userDefaults?.object(forKey: "\(keyPrefix)\(key)") as? T {
                option.value = value
            }
        }
        
        // General
        // TODO: Search engine migration
        migrate(key: "saveLogins", option: Preferences.General.saveLogins)
        migrate(key: "blockPopups", option: Preferences.General.blockPopups)
        migrate(key: "kPrefKeyTabsBarShowPolicy", option: Preferences.General.tabBarVisibility)
        migrate(key: "thirdPartyPasswordShortcutEnabled", option: Preferences.General.passwordManagerShortcutBehavior)
        
        // Privacy
        migrate(key: "braveAcceptCookiesPref", option: Preferences.Privacy.cookieAcceptPolicy)
        migrate(key: "privateBrowsingAlwaysOn", option: Preferences.Privacy.privateBrowsingOnly)
        
        // Security
        NSKeyedUnarchiver.setClass(AuthenticationKeychainInfo.self, forClassName: "AuthenticationKeychainInfo")
        if let pinLockInfo = KeychainWrapper.standard.object(forKey: "pinLockInfo") as? AuthenticationKeychainInfo {
            KeychainWrapper.sharedAppContainerKeychain.setAuthenticationInfo(pinLockInfo)
        }
        
        // Shields
        migrate(key: "braveBlockAdsAndTracking", option: Preferences.Shields.blockAdsAndTracking)
        migrate(key: "braveHttpsEverywhere", option: Preferences.Shields.httpsEverywhere)
        migrate(key: "braveSafeBrowsing", option: Preferences.Shields.blockPhishingAndMalware)
        migrate(key: "noscript_on", option: Preferences.Shields.blockScripts)
        migrate(key: "fingerprintprotection_on", option: Preferences.Shields.fingerprintingProtection)
        migrate(key: "braveAdblockUseRegional", option: Preferences.Shields.useRegionAdBlock)
        
        // Support
        migrate(key: "userallowstelemetry", option: Preferences.Support.sendsCrashReportsAndMetrics)
        
        Preferences.Migration.completed.value = true
    }
}
