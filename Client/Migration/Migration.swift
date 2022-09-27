/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import SwiftKeychainWrapper
import Data
import BraveCore

private let log = Logger.browserLogger

public class Migration {

  private(set) public var braveCoreSyncObjectsMigrator: BraveCoreMigrator?
  private let braveCore: BraveCoreMain

  public init(braveCore: BraveCoreMain) {
    self.braveCore = braveCore
  }

  public static var isChromiumMigrationCompleted: Bool {
    return Preferences.Chromium.syncV2BookmarksMigrationCompleted.value && Preferences.Chromium.syncV2HistoryMigrationCompleted.value && Preferences.Chromium.syncV2PasswordMigrationCompleted.value
  }

  public func launchMigrations(keyPrefix: String, profile: Profile) {
    Preferences.migratePreferences(keyPrefix: keyPrefix)
    
    Preferences.migrateWalletPreferences()

    if !Preferences.Migration.documentsDirectoryCleanupCompleted.value {
      documentsDirectoryCleanup()
      Preferences.Migration.documentsDirectoryCleanupCompleted.value = true
    }

    // `.migrate` is called in `BrowserViewController.viewDidLoad()`
    if !Migration.isChromiumMigrationCompleted {
      braveCoreSyncObjectsMigrator = BraveCoreMigrator(braveCore: braveCore, profile: profile)
    }

    if !Preferences.Migration.playlistV1FileSettingsLocationCompleted.value {
      movePlaylistV1Items()
    }
    
    if Preferences.General.isFirstLaunch.value {
      // Default Value for preference of tab bar visibility for new users changed to landscape only
      Preferences.General.tabBarVisibility.value = TabBarVisibility.landscapeOnly.rawValue
      // Default url bar location for new users is bottom
      Preferences.General.isUsingBottomBar.value = true
    }

    // Adding Observer to enable sync types

    NotificationCenter.default.addObserver(
      self,
      selector: #selector(enableUserSelectedTypesForSync),
      name: BraveServiceStateObserver.coreServiceLoadedNotification,
      object: nil
    )
  }

  public func moveDatabaseToApplicationDirectory() {
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

  @objc private func enableUserSelectedTypesForSync() {
    guard braveCore.syncAPI.isInSyncGroup else {
      log.info("Sync is not active")
      return
    }

    braveCore.syncAPI.enableSyncTypes(syncProfileService: braveCore.syncProfileService)
  }

  /// Adblock files don't have to be moved, they now have a new directory and will be downloaded there.
  /// Downloads folder was nefer used before, it's a leftover from FF.
  private func documentsDirectoryCleanup() {
    FileManager.default.removeFolder(withName: "abp-data", location: .documentDirectory)
    FileManager.default.removeFolder(withName: "https-everywhere-data", location: .documentDirectory)

    FileManager.default.moveFile(
      sourceName: "CookiesData.json", sourceLocation: .documentDirectory,
      destinationName: "CookiesData.json",
      destinationLocation: .applicationSupportDirectory)
  }

  private func movePlaylistV1Items() {
    // If moving the file fails, we'll never bother trying again.
    // It doesn't hurt and the user can easily delete it themselves.
    defer {
      Preferences.Migration.playlistV1FileSettingsLocationCompleted.value = true
    }

    guard let libraryPath = FileManager.default.urls(for: .libraryDirectory, in: .userDomainMask).first,
      let playlistDirectory = PlaylistDownloadManager.playlistDirectory
    else {
      return
    }

    let errorPath = "PlaylistV1FileSettingsLocationCompletion"
    do {
      let urls = try FileManager.default.contentsOfDirectory(
        at: libraryPath,
        includingPropertiesForKeys: nil,
        options: [.skipsHiddenFiles])
      for url in urls where url.absoluteString.contains("com.apple.UserManagedAssets") {
        do {
          let assets = try FileManager.default.contentsOfDirectory(
            at: url,
            includingPropertiesForKeys: nil,
            options: [.skipsHiddenFiles])
          assets.forEach({
            if let item = PlaylistItem.cachedItem(cacheURL: $0),
              let itemId = item.uuid {
              let destination = playlistDirectory.appendingPathComponent($0.lastPathComponent)

              do {
                try FileManager.default.moveItem(at: $0, to: destination)
                try PlaylistItem.updateCache(uuid: itemId, cachedData: destination.bookmarkData())
              } catch {
                log.error("Moving Playlist Item for \(errorPath) failed: \(error)")
              }
            }
          })
        } catch {
          log.error("Moving Playlist Item for \(errorPath) failed: \(error)")
        }

        do {
          try FileManager.default.removeItem(at: url)
        } catch {
          log.error("Deleting Playlist Item for \(errorPath) failed: \(error)")
        }
      }
    } catch {
      log.error("Moving Playlist Files for \(errorPath) failed: \(error)")
    }
  }

  private static func movePlaylistV2Items() {
    // Migrate all items not belonging to a folder
    func migrateItemsToSavedFolder(folderUUID: String) {
      let items = PlaylistItem.getItems(parentFolder: nil)
      if !items.isEmpty {
        PlaylistItem.moveItems(items: items.map({ $0.objectID }), to: folderUUID)
      }

      Preferences.Migration.playlistV2FoldersInitialMigrationCompleted.value = true
    }

    if PlaylistFolder.getFolder(uuid: PlaylistFolder.savedFolderUUID) != nil {
      migrateItemsToSavedFolder(folderUUID: PlaylistFolder.savedFolderUUID)
    } else {
      PlaylistFolder.addFolder(title: Strings.PlaylistFolders.playlistSavedFolderTitle, uuid: PlaylistFolder.savedFolderUUID) { uuid in
        if PlaylistFolder.getFolder(uuid: uuid) != nil {
          migrateItemsToSavedFolder(folderUUID: uuid)
        } else {
          log.error("Failed Moving Playlist items to Saved Folder - Unknown Error")
        }
      }
    }
  }
  
  private static func playlistFolderSharingIdentifierV2Migration() {
    let itemIDs = PlaylistItem.all().map({ $0.objectID })
    DataController.performOnMainContext(save: true) { context in
      let items = itemIDs.compactMap({ context.object(with: $0) as? PlaylistItem })
      items.forEach({
        if $0.uuid == nil {
          $0.uuid = UUID().uuidString
        }
      })
      
      Preferences.Migration.playlistV2SharedFoldersInitialMigrationCompleted.value = true
    }
  }

  public static func postCoreDataInitMigrations() {
    if !Preferences.Migration.removeLargeFaviconsMigrationCompleted.value {
      FaviconMO.clearTooLargeFavicons()
      Preferences.Migration.removeLargeFaviconsMigrationCompleted.value = true
    }

    if !Preferences.Migration.playlistV2FoldersInitialMigrationCompleted.value {
      movePlaylistV2Items()
    }
    
    if !Preferences.Migration.playlistV2SharedFoldersInitialMigrationCompleted.value {
      playlistFolderSharingIdentifierV2Migration()
    }

    if Preferences.Migration.coreDataCompleted.value { return }

    // In 1.6.6 we included private tabs in CoreData (temporarely) until the user did one of the following:
    //  - Cleared private data
    //  - Exited Private Mode
    //  - The app was terminated (bug)
    // However due to a bug, some private tabs remain in the container. Since 1.7 removes `isPrivate` from TabMO,
    // we must dismiss any records that are private tabs during migration from Model7
    TabMO.deleteAllPrivateTabs()

    Domain.migrateShieldOverrides()
    LegacyBookmarksHelper.migrateBookmarkOrders()

    Preferences.Migration.coreDataCompleted.value = true
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
    static let playlistV2FoldersInitialMigrationCompleted =
      Option<Bool>(key: "migration.playlistv2-folders-initial-migration-2-completed", default: false)
    static let playlistV2SharedFoldersInitialMigrationCompleted =
      Option<Bool>(key: "migration.playlistv2-sharedfolders-initial-migration-2-completed", default: false)
    static let removeLargeFaviconsMigrationCompleted =
      Option<Bool>(key: "migration.remove-large-favicons", default: false)
    // This is new preference introduced in iOS 1.32.3, tracks whether we should perform database migration.
    // It should be called only for users who have not completed the migration beforehand.
    // The reason for second migration flag is to first do file system migrations like moving database files,
    // then do CRUD operations on the db if needed.
    static let coreDataCompleted = Option<Bool>(
      key: "migration.cd-completed",
      default: Preferences.Migration.completed.value)
    /// A new preference key will be introduced in 1.44.x, indicates if Wallet Preferences migration has completed
    static let walletProviderAccountRequestCompleted =
    Option<Bool>(key: "migration.wallet-provider-account-request-completed", default: false)
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

    Preferences.Migration.completed.value = true
  }
  
  /// Migrate Wallet Preferences from version <1.43
  class func migrateWalletPreferences() {
    guard Preferences.Migration.walletProviderAccountRequestCompleted.value != true else { return }
    
    // Migrate `allowDappProviderAccountRequests` to `allowEthProviderAccess`
    migrate(keyPrefix: "", key: "wallet.allow-eth-provider-account-requests", to: Preferences.Wallet.allowEthProviderAccess)
    
    Preferences.Migration.walletProviderAccountRequestCompleted.value = true
  }
}
