/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import SwiftKeychainWrapper
import Data

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
        
        // Solely for 1.6.6 -> 1.7 migration
        if let pinLockInfo = KeychainWrapper.standard.object(forKey: "pinLockInfo") as? AuthenticationKeychainInfo {
            
            // Checks if browserLock was enabled in old app (1.6.6)
            let browserLockKey = "\(keyPrefix)browserLock"
            let isBrowserLockEnabled = Preferences.defaultContainer.bool(forKey: browserLockKey)
            
            // Preference no longer controls passcode functionality, so delete it
            Preferences.defaultContainer.removeObject(forKey: browserLockKey)
            if isBrowserLockEnabled {
                KeychainWrapper.sharedAppContainerKeychain.setAuthenticationInfo(pinLockInfo)
            }
            
            // No longer use this key, so remove, rely on other mechanisms
            KeychainWrapper.standard.removeObject(forKey: "pinLockInfo")
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
        
        // On 1.6 lastLaunchInfo is used to check if it's first app launch or not.
        // This needs to be translated to our new preference.
        Preferences.General.isFirstLaunch.value = Preferences.DAU.lastLaunchInfo.value == nil
        
        // Core Data
        
        // In 1.6.6 we included private tabs in CoreData (temporarely) until the user did one of the following:
        //  - Cleared private data
        //  - Exited Private Mode
        //  - The app was terminated (bug)
        // However due to a bug, some private tabs remain in the container. Since 1.7 removes `isPrivate` from TabMO,
        // we must dismiss any records that are private tabs during migration from Model7
        TabMO.deleteAll(predicate: NSPredicate(format: "isPrivate == true"), save: true)
        
        // Migrate the shield overrides
        migrateShieldOverrides()
        
        Bookmark.migrateOrder(forFavorites: true)
        Bookmark.migrateOrder(forFavorites: false)
        
        Preferences.Migration.completed.value = true
    }
    
    private class func migrateShieldOverrides() {
        // 1.6 had an unfortunate bug that caused shield overrides to create new Domain objects using `http` regardless
        // which would lead to a duplicated Domain object using http that held custom shield overides settings for that
        // domain
        //
        // Therefore we need to migrate any `http` Domains shield overrides to its sibiling `https` domain if it exists
        let allHttpPredicate = NSPredicate(format: "url BEGINSWITH[cd] 'http://'")
        let backgroundContext = DataController.newBackgroundContext()
        
        guard let httpDomains = Domain.all(where: allHttpPredicate, context: backgroundContext) else {
            return
        }
        
        for domain in httpDomains {
            guard let urlString = domain.url, var urlComponents = URLComponents(string: urlString) else { continue }
            urlComponents.scheme = "https"
            guard let httpsUrl = urlComponents.url?.absoluteString else { continue }
            if let httpsDomain = Domain.first(where: NSPredicate(format: "url == %@", httpsUrl), context: backgroundContext) {
                httpsDomain.shield_allOff = domain.shield_allOff
                httpsDomain.shield_adblockAndTp = domain.shield_adblockAndTp
                httpsDomain.shield_noScript = domain.shield_noScript
                httpsDomain.shield_fpProtection = domain.shield_fpProtection
                httpsDomain.shield_safeBrowsing = domain.shield_safeBrowsing
                httpsDomain.shield_httpse = domain.shield_httpse
                // Could call `domain.delete()` here (or add to batch to delete)
            }
        }
        DataController.save(context: backgroundContext)
    }
}
