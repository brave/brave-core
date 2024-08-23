// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Data
import Foundation
import Growth
import Preferences
import Shared
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
    Preferences.migrateHTTPSUpgradeLevel()
    Preferences.migrateBackgroundSponsoredImages()

    if Preferences.General.isFirstLaunch.value {
      if UIDevice.current.userInterfaceIdiom == .phone {
        // Default Value for preference of tab bar visibility for new users changed to landscape only
        Preferences.General.tabBarVisibility.value = TabBarVisibility.landscapeOnly.rawValue
      }
      // Default url bar location for new users is bottom
      Preferences.General.isUsingBottomBar.value = true
      Preferences.Playlist.firstLoadAutoPlay.value = true
    }

    migrateDeAmpPreferences()
    migrateDebouncePreferences()

    // Adding Observer to enable sync types
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(enableUserSelectedTypesForSync),
      name: BraveServiceStateObserver.coreServiceLoadedNotification,
      object: nil
    )
  }

  private func migrateDeAmpPreferences() {
    guard let isDeAmpEnabled = Preferences.Shields.autoRedirectAMPPagesDeprecated.value else {
      return
    }
    braveCore.deAmpPrefs.isDeAmpEnabled = isDeAmpEnabled
    Preferences.Shields.autoRedirectAMPPagesDeprecated.value = nil
  }

  private func migrateDebouncePreferences() {
    guard let isDebounceEnabled = Preferences.Shields.autoRedirectTrackingURLsDeprecated.value
    else {
      return
    }
    let debounceService = DebounceServiceFactory.get(privateMode: false)
    debounceService?.isEnabled = isDebounceEnabled
    Preferences.Shields.autoRedirectTrackingURLsDeprecated.value = nil
  }

  @objc private func enableUserSelectedTypesForSync() {
    guard braveCore.syncAPI.isInSyncGroup else {
      Logger.module.info("Sync is not active")
      return
    }

    braveCore.syncAPI.enableSyncTypes(syncProfileService: braveCore.syncProfileService)
  }

  public static func migrateLostTabsActiveWindow() {
    if UIApplication.shared.supportsMultipleScenes { return }
    if Preferences.Migration.lostTabsWindowIDMigration.value { return }

    var sessionWindows = SessionWindow.all()
    var activeWindow = sessionWindows.first(where: { $0.isSelected })
    if activeWindow == nil {
      activeWindow = sessionWindows.removeFirst()
    }

    guard let activeWindow = activeWindow else {
      Preferences.Migration.lostTabsWindowIDMigration.value = true
      return
    }

    let windowIds = UIApplication.shared.openSessions
      .compactMap({ BrowserState.getWindowInfo(from: $0).windowId })
      .filter({ $0 != activeWindow.windowId.uuidString })

    let zombieTabs =
      sessionWindows
      .filter({ windowIds.contains($0.windowId.uuidString) })
      .compactMap({
        $0.sessionTabs
      })
      .flatMap({ $0 })

    if !zombieTabs.isEmpty {
      let activeURLs = activeWindow.sessionTabs?.compactMap({ $0.url }) ?? []

      // Restore private tabs if persistency is enabled
      if Preferences.Privacy.persistentPrivateBrowsing.value {
        zombieTabs.filter({ $0.isPrivate }).forEach {
          if let url = $0.url, !activeURLs.contains(url) {
            SessionTab.move(tab: $0.tabId, toWindow: activeWindow.windowId)
          }
        }
      }

      // Restore regular tabs
      zombieTabs.filter({ !$0.isPrivate }).forEach {
        if let url = $0.url, !activeURLs.contains(url) {
          SessionTab.move(tab: $0.tabId, toWindow: activeWindow.windowId)
        }
      }
    }

    Preferences.Migration.lostTabsWindowIDMigration.value = true
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

    if !fm.fileExists(atPath: ledgerStateContainer.path)
      || fm.fileExists(atPath: adsConfirmations.path)
    {
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
      adsRewardsLog.error(
        "Failed to migrate confirmations.json to ads folder: \(error.localizedDescription)"
      )
    }
  }
}

extension Migration {
  /// Migrations that need to be run after data is loaded
  @MainActor public static func postDataLoadMigration() {
    migrateShieldLevel()
  }

  /// Migrate the shield level from the previous on/off toggle to the new ShieldLevel picker
  @MainActor private static func migrateShieldLevel() {
    guard
      !Preferences.Migration
        .domainAdBlockAndTrackingProtectionShieldLevelCompleted.value
    else {
      return
    }
    let domains = Domain.allDomainsWithMigratableShieldLevel()
    for domain in domains ?? [] {
      domain.migrateShieldLevel()
    }
    Preferences.Migration
      .domainAdBlockAndTrackingProtectionShieldLevelCompleted.value = true
  }
}

extension Preferences {
  private final class DeprecatedPreferences {
    static let blockAdsAndTracking = Option<Bool>(
      key: "shields.block-ads-and-tracking",
      default: true
    )

    /// Websites will be upgraded to HTTPS if a loaded page attempts to use HTTP
    public static let httpsEverywhere = Option<Bool>(
      key: "shields.https-everywhere",
      default: true
    )

    /// Whether sponsored images are included into the background image rotation
    static let backgroundSponsoredImages = Option<Bool>(
      key: "newtabpage.background-sponsored-images",
      default: true
    )
  }

  /// Migration preferences
  fileprivate final class Migration {
    static let completed = Option<Bool>(key: "migration.completed", default: false)

    /// A new preference key will be introduced in 1.44.x, indicates if Wallet Preferences migration has completed
    static let walletProviderAccountRequestCompleted =
      Option<Bool>(key: "migration.wallet-provider-account-request-completed", default: false)

    static let tabMigrationToInteractionStateCompleted = Option<Bool>(
      key: "migration.tab-to-interaction-state",
      default: false
    )

    /// A more complicated ad blocking and tracking protection preference  in `1.52.x`
    /// allows a user to select between `standard`, `aggressive` and `disabled` instead of a simple on/off `Bool`
    static let adBlockAndTrackingProtectionShieldLevelCompleted = Option<Bool>(
      key: "migration.ad-block-and-tracking-protection-shield-level-completed",
      default: false
    )

    /// A per domain ad blocking and tracking protection preference  in `1.69.x`
    /// allows a user to select between `standard`, `aggressive` and `disabled`
    /// instead of a simple on/off `Bool` on the domain level
    static let domainAdBlockAndTrackingProtectionShieldLevelCompleted = Option<Bool>(
      key: "migration.domain-ad-block-and-tracking-protection-shield-level-completed",
      default: false
    )

    /// A more complicated https upgrades preference
    /// allows a user to select between `standard`, `strict` and `disabled` instead of a simple on/off `Bool`
    static let httpsUpgradesLivelCompleted = Option<Bool>(
      key: "migration.https-upgrades-level-completed",
      default: false
    )

    static let lostTabsWindowIDMigration = Option<Bool>(
      key: "migration.lost-tabs-window-id-two",
      default: !UIApplication.shared.supportsMultipleScenes
    )

    static let backgroundSponsoredImagesCompleted = Option<Bool>(
      key: "migration.newtabpage-background-sponsored-images",
      default: false
    )
  }

  /// Migrate a given key from `Prefs` into a specific option
  fileprivate class func migrate<T>(
    keyPrefix: String,
    key: String,
    to option: Preferences.Option<T>,
    transform: ((T) -> T)? = nil
  ) {
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
  fileprivate class func migratePreferences(keyPrefix: String) {
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
        try FileManager.default.setAttributes(
          [.posixPermissions: NSNumber(value: 0o755 as Int16)],
          ofItemAtPath: $0
        )
      } catch {
        Logger.module.error("Failed setting the directory attributes for \($0)")
      }
    }

    // Shields
    migrate(key: "braveBlockAdsAndTracking", to: DeprecatedPreferences.blockAdsAndTracking)
    migrate(key: "braveHttpsEverywhere", to: DeprecatedPreferences.httpsEverywhere)
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
    migrate(key: "fp_protection", to: Preferences.BlockStats.fingerprintingCount)
    migrate(key: "safebrowsing", to: Preferences.BlockStats.phishingCount)

    // On 1.6 lastLaunchInfo is used to check if it's first app launch or not.
    // This needs to be translated to our new preference.
    Preferences.General.isFirstLaunch.value = Preferences.DAU.lastLaunchInfo.value == nil
    Preferences.Migration.completed.value = true
  }

  fileprivate class func migrateAdAndTrackingProtection() {
    guard !Migration.adBlockAndTrackingProtectionShieldLevelCompleted.value else { return }

    // Migrate old tracking protection setting to new BraveShields setting
    DeprecatedPreferences.blockAdsAndTracking.migrate { isEnabled in
      if !isEnabled {
        // We only need to migrate `disabled`. `standard` is the default.
        ShieldPreferences.blockAdsAndTrackingLevel = .disabled
      }
    }

    Migration.adBlockAndTrackingProtectionShieldLevelCompleted.value = true
  }

  fileprivate class func migrateHTTPSUpgradeLevel() {
    // If the feature flag for https by default is off but we've already stored a user pref for it
    // then assign that enabled level a preference so that if a user toggles HTTPS Everywhere off
    // and on it will correctly set the underlying upgarde level to the level they had set when
    // the feature flag was on.
    if !FeatureList.kBraveHttpsByDefault.enabled || !FeatureList.kHttpsOnlyMode.enabled,
      ShieldPreferences.httpsUpgradeLevel.isEnabled
    {
      ShieldPreferences.httpsUpgradePriorEnabledLevel = ShieldPreferences.httpsUpgradeLevel
    }
    guard !Migration.httpsUpgradesLivelCompleted.value else { return }

    // Migrate old tracking protection setting to new BraveShields setting
    DeprecatedPreferences.httpsEverywhere.migrate { isEnabled in
      ShieldPreferences.httpsUpgradeLevel = isEnabled ? .standard : .disabled
    }

    Migration.adBlockAndTrackingProtectionShieldLevelCompleted.value = true
  }

  /// Migrate Wallet Preferences from version <1.43
  fileprivate class func migrateWalletPreferences() {
    guard Preferences.Migration.walletProviderAccountRequestCompleted.value != true else { return }

    // Migrate `allowDappProviderAccountRequests` to `allowEthProviderAccess`
    migrate(
      keyPrefix: "",
      key: "wallet.allow-eth-provider-account-requests",
      to: Preferences.Wallet.allowEthProviderAccess
    )

    Preferences.Migration.walletProviderAccountRequestCompleted.value = true
  }

  fileprivate class func migrateBackgroundSponsoredImages() {
    guard !Migration.backgroundSponsoredImagesCompleted.value else { return }

    // Migrate old Background Sponsored Images setting
    DeprecatedPreferences.backgroundSponsoredImages.migrate { isEnabled in
      Preferences.NewTabPage.backgroundMediaType =
        isEnabled ? .sponsoredImagesAndVideos : .defaultImages
    }

    Migration.backgroundSponsoredImagesCompleted.value = true
  }
}

extension Preferences.Option {
  /// Migrate this preference to another one using the given transform
  ///
  /// This method will return any stored value (if it is available). If nothing is stored, the callback is not triggered.
  /// Any stored value is then removed from the container.
  fileprivate func migrate(onStoredValue: ((ValueType) -> Void)) {
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
