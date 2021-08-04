/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import SwiftKeychainWrapper
import Data
import BraveRewards

private let log = Logger.browserLogger

class Migration {
    
    private(set) public static var braveCoreSyncObjectsMigrator: BraveCoreMigrator?

    public static var isChromiumMigrationCompleted: Bool {
        return Preferences.Chromium.syncV2BookmarksMigrationCompleted.value &&
            Preferences.Chromium.syncV2HistoryMigrationCompleted.value
    }
    
    static func launchMigrations(keyPrefix: String) {
        Preferences.migratePreferences(keyPrefix: keyPrefix)
        
        if !Preferences.Migration.documentsDirectoryCleanupCompleted.value {
            documentsDirectoryCleanup()
            Preferences.Migration.documentsDirectoryCleanupCompleted.value = true
        }
        
        // `.migrate` is called in `BrowserViewController.viewDidLoad()`
        if !isChromiumMigrationCompleted {
            braveCoreSyncObjectsMigrator = BraveCoreMigrator()
        }
        
        if !Preferences.Migration.playlistV1FileSettingsLocationCompleted.value {
            movePlaylistV1Items()
        }
        
        if !Preferences.Migration.removeLargeFaviconsMigrationCompleted.value {
            FaviconMO.clearTooLargeFavicons()
            Preferences.Migration.removeLargeFaviconsMigrationCompleted.value = true
        }

        // Adding Observer to enable sync types
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(enableUserSelectedTypesForSync),
            name: BraveServiceStateObserver.coreServiceLoadedNotification,
            object: nil
        )
    }
    
    @objc private func enableUserSelectedTypesForSync() {
        guard BraveSyncAPI.shared.isInSyncGroup else {
            log.info("Sync is not active")
            return
        }
        
        BraveSyncAPI.shared.enableSyncTypes()
    }
    
    static func moveDatabaseToApplicationDirectory() {
        if Preferences.Database.DocumentToSupportDirectoryMigration.completed.value {
            // Migration has been done in some regard, so drop out.
            return
        }
        
        if Preferences.Database.DocumentToSupportDirectoryMigration.previousAttemptedVersion.value == AppInfo.appVersion {
            // Migration has already been attempted for this version.
            return
        }
        
        // Moves Coredata sqlite file from documents dir to application support dir.
        do {
            try DataController.shared.migrateToNewPathIfNeeded()
        } catch {
            log.error(error)
        }
        
        // Regardless of what happened, we attemtped a migration and document it:
        Preferences.Database.DocumentToSupportDirectoryMigration.previousAttemptedVersion.value = AppInfo.appVersion
    }
    
    /// Adblock files don't have to be moved, they now have a new directory and will be downloaded there.
    /// Downloads folder was nefer used before, it's a leftover from FF.
    private static func documentsDirectoryCleanup() {
        FileManager.default.removeFolder(withName: "abp-data", location: .documentDirectory)
        FileManager.default.removeFolder(withName: "https-everywhere-data", location: .documentDirectory)
        
        FileManager.default.moveFile(sourceName: "CookiesData.json", sourceLocation: .documentDirectory,
                                     destinationName: "CookiesData.json",
                                     destinationLocation: .applicationSupportDirectory)
    }
    
    private static func movePlaylistV1Items() {
        // If moving the file fails, we'll never bother trying again.
        // It doesn't hurt and the user can easily delete it themselves.
        defer {
            Preferences.Migration.playlistV1FileSettingsLocationCompleted.value = true
        }
        
        guard let libraryPath = FileManager.default.urls(for: .libraryDirectory, in: .userDomainMask).first,
              let playlistDirectory = PlaylistDownloadManager.playlistDirectory else {
            return
        }

        let errorPath = "PlaylistV1FileSettingsLocationCompletion"
        do {
            let urls = try FileManager.default.contentsOfDirectory(at: libraryPath,
                                                                   includingPropertiesForKeys: nil,
                                                                   options: [.skipsHiddenFiles])
            for url in urls where url.absoluteString.contains("com.apple.UserManagedAssets") {
                do {
                    let assets = try FileManager.default.contentsOfDirectory(at: url,
                                                                             includingPropertiesForKeys: nil,
                                                                             options: [.skipsHiddenFiles])
                    assets.forEach({
                        if let item = PlaylistItem.cachedItem(cacheURL: $0),
                           let pageSrc = item.pageSrc {
                            let destination = playlistDirectory.appendingPathComponent($0.lastPathComponent)
                            
                            do {
                                try FileManager.default.moveItem(at: $0, to: destination)
                                try PlaylistItem.updateCache(pageSrc: pageSrc, cachedData: destination.bookmarkData())
                            } catch {
                                log.error("Moving Playlist Item for \(errorPath) failed: \(error)")
                            }
                        }
                    })
                } catch {
                    log.error("Moving Playlist Item for \(errorPath) failed: \(error)")
                }
            }
        } catch {
            log.error("Moving Playlist Files for \(errorPath) failed: \(error)")
        }
    }
}

fileprivate extension Preferences {
    /// Migration preferences
    final class Migration {
        static let completed = Option<Bool>(key: "migration.completed", default: false)
        /// Old app versions were using documents directory to store app files, database, adblock files.
        /// These files are now moved to 'Application Support' folder, and documents directory is left
        /// for user downloaded files.
        static let documentsDirectoryCleanupCompleted =
            Option<Bool>(key: "migration.documents-dir-completed", default: false)
        static let playlistV1FileSettingsLocationCompleted =
            Option<Bool>(key: "migration.playlistv1-file-settings-location-completed", default: false)
        static let removeLargeFaviconsMigrationCompleted =
            Option<Bool>(key: "migration.remove-large-favicons", default: false)
    }
    
    /// Migrate the users preferences from prior versions of the app (<2.0)
    class func migratePreferences(keyPrefix: String) {
        if Preferences.Migration.completed.value {
            return
        }
        
        /// Wrapper around BraveShared migrate, to automate prefix injection
        func migrate<T>(key: String, to option: Preferences.Option<T>, transform: ((T) -> T)? = nil) {
            self.migrate(keyPrefix: keyPrefix, key: key, to: option, transform: transform)
        }
        
        // General
        migrate(key: "saveLogins", to: Preferences.General.saveLogins)
        migrate(key: "blockPopups", to: Preferences.General.blockPopups)
        migrate(key: "kPrefKeyTabsBarShowPolicy", to: Preferences.General.tabBarVisibility)
        
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
        TabMO.deleteAllPrivateTabs()
        
        Domain.migrateShieldOverrides()
        LegacyBookmarksHelper.migrateBookmarkOrders()
        
        Preferences.Migration.completed.value = true
    }
}
