/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import Preferences
import BraveShields
import SwiftKeychainWrapper
import Data
import BraveCore
import Growth
import Storage
import os.log

public class Migration {

  private let braveCore: BraveCoreMain

  public init(braveCore: BraveCoreMain) {
    self.braveCore = braveCore
  }

  public func launchMigrations(keyPrefix: String, profile: Profile) {
    Preferences.migratePreferences(keyPrefix: keyPrefix)
    Preferences.migrateWalletPreferences()
    Preferences.migrateAdAndTrackingProtection()

    if !Preferences.Migration.documentsDirectoryCleanupCompleted.value {
      documentsDirectoryCleanup()
      Preferences.Migration.documentsDirectoryCleanupCompleted.value = true
    }
    
    if Preferences.General.isFirstLaunch.value {
      if UIDevice.current.userInterfaceIdiom == .phone {
        // Default Value for preference of tab bar visibility for new users changed to landscape only
        Preferences.General.tabBarVisibility.value = TabBarVisibility.landscapeOnly.rawValue
      }
      // Default url bar location for new users is bottom
      Preferences.General.isUsingBottomBar.value = true
      Preferences.Playlist.firstLoadAutoPlay.value = true
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
    guard braveCore.syncAPI.isInSyncGroup else {
      Logger.module.info("Sync is not active")
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
  
  // Migrate from TabMO to SessionTab and SessionWindow
  public static func migrateTabStateToWebkitState(diskImageStore: DiskImageStore?) {
    let isPrivate = false // Private tabs at the time of writing this code was never persistent, so it wouldn't "restore".

    if Preferences.Migration.tabMigrationToInteractionStateCompleted.value {
      SessionWindow.createIfNeeded(index: 0, isPrivate: isPrivate, isSelected: true)
      return
    }
    
    // Get all the old Tabs from TabMO
    let oldTabIDs = TabMO.getAll().map({ $0.objectID })

    // Nothing to migrate
    if oldTabIDs.isEmpty {
      // Create a SessionWindow (default window)
      // Set the window selected by default
      TabMO.migrate { context in
        _ = SessionWindow(context: context, index: 0, isPrivate: isPrivate, isSelected: true)
      }
      
      Preferences.Migration.tabMigrationToInteractionStateCompleted.value = true
      return
    }
    
    TabMO.migrate { context in
      let oldTabs = oldTabIDs.compactMap({ context.object(with: $0) as? TabMO })
      if oldTabs.isEmpty { return }  // Migration failed

      // Create a SessionWindow (default window)
      // Set the window selected by default
      let sessionWindow = SessionWindow(context: context, index: 0, isPrivate: isPrivate, isSelected: true)
      
      oldTabs.forEach { oldTab in
        guard let urlString = oldTab.url,
              let url = NSURL(idnString: urlString) as? URL ?? URL(string: urlString) else {
          return
        }
        
        var tabId: UUID
        if let syncUUID = oldTab.syncUUID {
          tabId = UUID(uuidString: syncUUID) ?? UUID()
        } else {
          tabId = UUID()
        }
        
        var historyURLs = [URL]()
        let tabTitle = oldTab.title ?? Strings.newTab
        let historySnapshot = oldTab.urlHistorySnapshot as? [String] ?? []
        
        for url in historySnapshot {
          guard let url = NSURL(idnString: url) as? URL ?? URL(string: url) else {
            Logger.module.error("Failed to parse URL: \(url) during Migration!")
            continue
          }
          if let internalUrl = InternalURL(url), !internalUrl.isAuthorized, let authorizedURL = InternalURL.authorize(url: url) {
            historyURLs.append(authorizedURL)
          } else {
            historyURLs.append(url)
          }
        }
        
        if historyURLs.count == 0 {
          Logger.module.error("User has zero history to migrate!")
          return
        }

        // currentPage is -webView.backForwardList.forwardList.count
        // If for some reason current page can be negative, we clamp it to [0, inf].
        let currentPage = max((historyURLs.count - 1) + Int(oldTab.urlHistoryCurrentIndex), 0)
        
        // Create WebKit interactionState
        let interactionState = SynthesizedSessionRestore.serialize(withTitle: tabTitle,
                                                                   historyURLs: historyURLs,
                                                                   pageIndex: UInt(currentPage),
                                                                   isPrivateBrowsing: isPrivate)
        
        var screenshot = Data()
        if let imageStore = diskImageStore, let screenshotUUID = oldTab.screenshotUUID {
          do {
            let image = try imageStore.getSynchronously(screenshotUUID)
            if let data = image.jpegData(compressionQuality: CGFloat(UIConstants.screenshotQuality)) {
              screenshot = data
            }
          } catch {
            Logger.module.error("Failed to migrate screenshot for Tab: \(error)")
          }
        }
        
        // Create SessionTab and associate it with a SessionWindow
        // Tabs currently do not have groups, so sessionTabGroup is nil by default
        _ = SessionTab(context: context,
                       sessionWindow: sessionWindow,
                       sessionTabGroup: nil,
                       index: Int32(oldTab.order),
                       interactionState: interactionState,
                       isPrivate: isPrivate,
                       isSelected: oldTab.isSelected,
                       lastUpdated: oldTab.lastUpdate ?? .now,
                       screenshotData: screenshot,
                       title: tabTitle,
                       url: url,
                       tabId: tabId)
      }
      
      Preferences.Migration.tabMigrationToInteractionStateCompleted.value = true
    }
  }

  public static func postCoreDataInitMigrations() {
    if Preferences.Migration.coreDataCompleted.value { return }

    TabMO.deleteAllPrivateTabs()
  
    Domain.migrateShieldOverrides()
  
    Preferences.Migration.coreDataCompleted.value = true
  }
  
  public static func migrateAdsConfirmations(for configruation: BraveRewards.Configuration) {
    // To ensure after a user launches 1.21 that their ads confirmations, viewed count and
    // estimated payout remain correct.
    //
    // This hack is unfortunately neccessary due to a missed migration path when moving
    // confirmations from ledger to ads, we must extract `confirmations.json` out of ledger's
    // state file and save it as a new file under the ads directory.
    let base = configruation.storageURL
    let ledgerStateContainer = base.appendingPathComponent("ledger/random_state.plist")
    let adsConfirmations = base.appendingPathComponent("ads/confirmations.json")
    let fm = FileManager.default

    if !fm.fileExists(atPath: ledgerStateContainer.path) || fm.fileExists(atPath: adsConfirmations.path) {
      // Nothing to migrate or already migrated
      return
    }

    do {
      let contents = NSDictionary(contentsOfFile: ledgerStateContainer.path)
      guard let confirmations = contents?["confirmations.json"] as? String else {
        adsRewardsLog.debug("No confirmations found to migrate in ledger's state container")
        return
      }
      try confirmations.write(toFile: adsConfirmations.path, atomically: true, encoding: .utf8)
    } catch {
      adsRewardsLog.error("Failed to migrate confirmations.json to ads folder: \(error.localizedDescription)")
    }
  }
}

fileprivate extension Preferences {
  private final class DeprecatedPreferences {
    static let blockAdsAndTracking = Option<Bool>(key: "shields.block-ads-and-tracking", default: true)
  }
  
  /// Migration preferences
  final class Migration {
    static let completed = Option<Bool>(key: "migration.completed", default: false)
    /// Old app versions were using documents directory to store app files, database, adblock files.
    /// These files are now moved to 'Application Support' folder, and documents directory is left
    /// for user downloaded files.
    static let documentsDirectoryCleanupCompleted =
      Option<Bool>(key: "migration.documents-dir-completed", default: false)
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

    static let tabMigrationToInteractionStateCompleted = Option<Bool>(key: "migration.tab-to-interaction-state", default: false)
    
    /// A more complicated ad blocking and tracking protection preference  in `1.52.x`
    /// allows a user to select between `standard`, `aggressive` and `disabled` instead of a simple on/off `Bool`
    static let adBlockAndTrackingProtectionShieldLevelCompleted = Option<Bool>(
      key: "migration.ad-block-and-tracking-protection-shield-level-completed", default: false
    )
  }

  /// Migrate a given key from `Prefs` into a specific option
  class func migrate<T>(keyPrefix: String, key: String, to option: Preferences.Option<T>, transform: ((T) -> T)? = nil) {
    let userDefaults = UserDefaults(suiteName: AppInfo.sharedContainerIdentifier)
    let profileKey = "\(keyPrefix)\(key)"
    // Have to do two checks because T may be an Optional, since object(forKey:) returns Any? it will succeed
    // as casting to T if T is Optional even if the key doesnt exist.
    let value = userDefaults?.object(forKey: profileKey)
    if value != nil, let value = value as? T {
      if let transform = transform {
        option.value = transform(value)
      } else {
        option.value = value
      }
      userDefaults?.removeObject(forKey: profileKey)
    } else {
      Logger.module.info("Could not migrate legacy pref with key: \"\(profileKey)\".")
    }
  }
  
  /// Migrate the users preferences from prior versions of the app (<2.0)
  class func migratePreferences(keyPrefix: String) {
    if Preferences.Migration.completed.value {
      return
    }

    // Wrapper around BraveShared migrate, to automate prefix injection
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
        Logger.module.error("Failed setting the directory attributes for \($0)")
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
    migrate(key: "braveBlockAdsAndTracking", to: DeprecatedPreferences.blockAdsAndTracking)
    migrate(key: "braveHttpsEverywhere", to: Preferences.Shields.httpsEverywhere)
    migrate(key: "noscript_on", to: Preferences.Shields.blockScripts)
    migrate(key: "fingerprintprotection_on", to: Preferences.Shields.fingerprintingProtection)
    migrate(key: "braveAdblockUseRegional", to: Preferences.Shields.useRegionAdBlock)

    // DAU
    migrate(key: "dau_stat", to: Preferences.DAU.lastLaunchInfo)
    migrate(key: "week_of_installation", to: Preferences.DAU.weekOfInstallation)

    // URP
    migrate(key: "urpDateCheckPrefsKey", to: Preferences.URP.nextCheckDate)
    migrate(key: "urpRetryCountdownPrefsKey", to: Preferences.URP.retryCountdown)
    migrate(key: "downloadIdPrefsKey", to: Preferences.URP.downloadId)
    migrate(key: "referralCodePrefsKey", to: Preferences.URP.referralCode)
    migrate(key: "referralCodeDeleteTimePrefsKey", to: Preferences.URP.referralCodeDeleteDate)

    // Block Stats
    migrate(key: "adblock", to: Preferences.BlockStats.adsCount)
    migrate(key: "tracking_protection", to: Preferences.BlockStats.trackersCount)
    migrate(key: "httpse", to: Preferences.BlockStats.httpsUpgradeCount)
    migrate(key: "fp_protection", to: Preferences.BlockStats.fingerprintingCount)
    migrate(key: "safebrowsing", to: Preferences.BlockStats.phishingCount)

    // On 1.6 lastLaunchInfo is used to check if it's first app launch or not.
    // This needs to be translated to our new preference.
    Preferences.General.isFirstLaunch.value = Preferences.DAU.lastLaunchInfo.value == nil
    Preferences.Migration.completed.value = true
  }
  
  class func migrateAdAndTrackingProtection() {
    guard !Migration.adBlockAndTrackingProtectionShieldLevelCompleted.value else { return }
    
    // Migrate old tracking protection setting to new BraveShields setting
    DeprecatedPreferences.blockAdsAndTracking.migrate() { isEnabled in
      if !isEnabled {
        // We only need to migrate `disabled`. `standard` is the default.
        ShieldPreferences.blockAdsAndTrackingLevel = .disabled
      }
    }
    
    Migration.adBlockAndTrackingProtectionShieldLevelCompleted.value = true
  }
  
  /// Migrate Wallet Preferences from version <1.43
  class func migrateWalletPreferences() {
    guard Preferences.Migration.walletProviderAccountRequestCompleted.value != true else { return }
    
    // Migrate `allowDappProviderAccountRequests` to `allowEthProviderAccess`
    migrate(keyPrefix: "", key: "wallet.allow-eth-provider-account-requests", to: Preferences.Wallet.allowEthProviderAccess)
    
    Preferences.Migration.walletProviderAccountRequestCompleted.value = true
  }
}

private extension Preferences.Option {
  /// Migrate this preference to another one using the given transform
  ///
  /// This method will return any stored value (if it is available). If nothing is stored, the callback is not triggered.
  /// Any stored value is then removed from the container.
  func migrate(onStoredValue: ((ValueType) -> Void)) {
    // Have to do two checks because T may be an Optional, since object(forKey:) returns Any? it will succeed
    // as casting to T if T is Optional even if the key doesnt exist.
    if let value = container.object(forKey: key) {
      if let value = value as? ValueType {
        onStoredValue(value)
      }
      
      container.removeObject(forKey: key)
    } else {
      Logger.module.info("Could not migrate legacy pref with key: \"\(self.key)\".")
    }
  }
}
