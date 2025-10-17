// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveNews
import BraveShared
import BraveShields
import Combine
import Data
import Foundation
import Growth
import Preferences
import Web
import os

@MainActor class AdvancedShieldsSettings: ObservableObject {
  struct ClearableSetting: Identifiable {
    enum ClearableType: String {
      case history, cache, cookiesAndCache, passwords, downloads, braveNews, playlistCache,
        playlistData, recentSearches, braveAdsData
    }

    var id: ClearableType
    var clearable: Clearable
    var isEnabled: Bool
  }

  @Published var blockMobileAnnoyances: Bool {
    didSet {
      FilterListStorage.shared.ensureFilterList(
        for: AdblockFilterListCatalogEntry.mobileAnnoyancesComponentID,
        isEnabled: blockMobileAnnoyances
      )
    }
  }
  var isP3AManaged: Bool {
    p3aUtilities.isP3APreferenceManaged
  }
  var isStatsReportingManaged: Bool {
    braveStats.isStatsReportingManaged
  }
  @Published var isStatsReportingEnabled: Bool {
    didSet {
      braveStats.isStatsReportingEnabled = isStatsReportingEnabled
    }
  }
  @Published var isP3AEnabled: Bool {
    didSet {
      p3aUtilities.isP3AEnabled = isP3AEnabled
    }
  }
  @Published var isDeAmpEnabled: Bool {
    didSet {
      deAmpPrefs.isDeAmpEnabled = isDeAmpEnabled
    }
  }
  @Published var isDebounceEnabled: Bool {
    didSet {
      guard debounceService != nil else {
        isDebounceEnabled = false
        return
      }

      debounceService?.isEnabled = isDebounceEnabled
    }
  }
  @Published var adBlockAndTrackingPreventionLevel: ShieldLevel {
    didSet {
      guard oldValue != adBlockAndTrackingPreventionLevel else { return }
      if FeatureList.kBraveShieldsContentSettings.enabled {
        braveShieldsSettings.defaultAdBlockMode = adBlockAndTrackingPreventionLevel.adBlockMode
      }
      // Also assign to existing pref until deprecated so reverse migration is not required
      ShieldPreferences.blockAdsAndTrackingLevel = adBlockAndTrackingPreventionLevel
    }
  }
  @Published var isBlockScriptsEnabled: Bool {
    didSet {
      guard oldValue != isBlockScriptsEnabled else { return }
      if FeatureList.kBraveShieldsContentSettings.enabled {
        braveShieldsSettings.isBlockScriptsEnabledByDefault = isBlockScriptsEnabled
      }
      // Also assign to existing pref until deprecated so reverse migration is not required
      Preferences.Shields.blockScripts.value = isBlockScriptsEnabled
    }
  }
  @Published var isBlockFingerprintingEnabled: Bool {
    didSet {
      guard oldValue != isBlockFingerprintingEnabled else { return }
      if FeatureList.kBraveShieldsContentSettings.enabled {
        braveShieldsSettings.defaultFingerprintMode =
          isBlockFingerprintingEnabled ? .standardMode : .allowMode
      }
      // Also assign to existing pref until deprecated so reverse migration is not required
      Preferences.Shields.fingerprintingProtection.value = isBlockFingerprintingEnabled
    }
  }
  @Published var httpsUpgradeLevel: HTTPSUpgradeLevel {
    didSet {
      ShieldPreferences.httpsUpgradeLevel = httpsUpgradeLevel
      HttpsUpgradeServiceFactory.get(privateMode: false)?.clearAllowlist(
        fromStart: Date.distantPast,
        end: Date.distantFuture
      )
    }
  }
  @Published var shredLevel: SiteShredLevel {
    didSet {
      // TODO: Support AutoShred via content settings brave-browser#47753
      ShieldPreferences.shredLevel = shredLevel
    }
  }
  @Published var shredHistoryItems: Bool {
    didSet {
      Preferences.Shields.shredHistoryItems.value = shredHistoryItems
    }
  }

  @Published var isSaveContactInfoEnabled: Bool = false {
    didSet {
      guard let webcompatReporterHandler else {
        isSaveContactInfoEnabled = false
        return
      }

      webcompatReporterHandler.setContactInfoSaveFlag(value: isSaveContactInfoEnabled)
    }
  }

  @Published var isSurveyPanelistEnabled: Bool = false {
    didSet {
      rewards?.ads.isSurveyPanelistEnabled = isSurveyPanelistEnabled
    }
  }

  typealias ClearDataCallback = @MainActor (Bool, Bool) -> Void
  @Published var clearableSettings: [ClearableSetting]

  private var subscriptions: [AnyCancellable] = []
  private let p3aUtilities: BraveP3AUtils
  private let deAmpPrefs: DeAmpPrefs
  private let debounceService: (any DebounceService)?
  private let braveShieldsSettings: any BraveShieldsSettings
  private let rewards: BraveRewards?
  private let clearDataCallback: ClearDataCallback
  private let braveStats: BraveStats
  private let webcompatReporterHandler: WebcompatReporterWebcompatReporterHandler?
  let tabManager: TabManager

  init(
    profile: LegacyBrowserProfile,
    tabManager: TabManager,
    feedDataSource: FeedDataSource,
    debounceService: (any DebounceService)?,
    braveShieldsSettings: any BraveShieldsSettings,
    braveCore: BraveProfileController,
    p3aUtils: BraveP3AUtils,
    rewards: BraveRewards?,
    braveStats: BraveStats,
    webcompatReporterHandler: WebcompatReporterWebcompatReporterHandler?,
    clearDataCallback: @escaping ClearDataCallback
  ) {
    self.p3aUtilities = p3aUtils
    self.deAmpPrefs = braveCore.deAmpPrefs
    self.debounceService = debounceService
    self.braveShieldsSettings = braveShieldsSettings
    self.tabManager = tabManager
    self.isP3AEnabled = p3aUtilities.isP3AEnabled
    self.isStatsReportingEnabled = braveStats.isStatsReportingEnabled
    self.rewards = rewards
    self.clearDataCallback = clearDataCallback
    self.braveStats = braveStats
    if FeatureList.kBraveShieldsContentSettings.enabled {
      self.adBlockAndTrackingPreventionLevel = braveShieldsSettings.defaultAdBlockMode.shieldLevel
      self.isBlockScriptsEnabled = braveShieldsSettings.isBlockScriptsEnabledByDefault
      self.isBlockFingerprintingEnabled =
        braveShieldsSettings.defaultFingerprintMode == .standardMode
    } else {
      self.adBlockAndTrackingPreventionLevel = ShieldPreferences.blockAdsAndTrackingLevel
      self.isBlockScriptsEnabled = Preferences.Shields.blockScripts.value
      self.isBlockFingerprintingEnabled = Preferences.Shields.fingerprintingProtection.value
    }
    self.httpsUpgradeLevel = ShieldPreferences.httpsUpgradeLevel
    self.isDeAmpEnabled = deAmpPrefs.isDeAmpEnabled
    self.isDebounceEnabled = debounceService?.isEnabled ?? false
    // TODO: Support AutoShred via content settings brave-browser#47753
    self.shredLevel = ShieldPreferences.shredLevel
    self.shredHistoryItems = Preferences.Shields.shredHistoryItems.value
    self.webcompatReporterHandler = webcompatReporterHandler
    self.isSurveyPanelistEnabled = rewards?.ads.isSurveyPanelistEnabled ?? false

    blockMobileAnnoyances = FilterListStorage.shared.isEnabled(
      for: AdblockFilterListCatalogEntry.mobileAnnoyancesComponentID
    )

    var clearableSettings = [
      ClearableSetting(
        id: .history,
        clearable: HistoryClearable(
          historyAPI: braveCore.historyAPI,
          httpsUpgradeService: HttpsUpgradeServiceFactory.get(privateMode: false)
        ),
        isEnabled: true
      ),
      ClearableSetting(id: .cache, clearable: CacheClearable(), isEnabled: true),
      ClearableSetting(
        id: .cookiesAndCache,
        clearable: CookiesAndCacheClearable(),
        isEnabled: true
      ),
      ClearableSetting(
        id: .passwords,
        clearable: PasswordsClearable(profile: profile),
        isEnabled: true
      ),
      ClearableSetting(id: .downloads, clearable: DownloadsClearable(), isEnabled: true),
    ]
    if braveCore.profile.prefs.isBraveNewsAvailable {
      clearableSettings.append(
        ClearableSetting(
          id: .braveNews,
          clearable: BraveNewsClearable(feedDataSource: feedDataSource),
          isEnabled: true
        )
      )
    }

    // Enable clearing of Brave Ads data only if:
    // - Brave Ads is running
    // - Brave Rewards is disabled
    if let rewards, !rewards.isEnabled, rewards.ads.isServiceRunning() {
      clearableSettings.append(
        ClearableSetting(
          id: .braveAdsData,
          clearable: BraveAdsDataClearable(rewards: rewards),
          isEnabled: false
        )
      )
    }

    if braveCore.profile.prefs.isPlaylistAvailable {
      clearableSettings += [
        ClearableSetting(id: .playlistCache, clearable: PlayListCacheClearable(), isEnabled: false),
        ClearableSetting(id: .playlistData, clearable: PlayListDataClearable(), isEnabled: false),
      ]
    }

    clearableSettings.append(
      ClearableSetting(id: .recentSearches, clearable: RecentSearchClearable(), isEnabled: true)
    )

    let savedToggles = Preferences.Privacy.clearPrivateDataToggles.value

    // Ensure if we ever add an option to the list of clearables we don't crash
    if savedToggles.count == clearableSettings.count {
      for index in 0..<clearableSettings.count {
        clearableSettings[index].isEnabled = savedToggles[index]
      }
    }

    self.clearableSettings = clearableSettings
    registerSubscriptions()
    Task { @MainActor in
      self.isSaveContactInfoEnabled = await webcompatReporterHandler?.browserParams().1 ?? false
    }
  }

  func clearPrivateData(_ clearables: [Clearable]) async {
    clearDataCallback(true, false)
    let isHistoryCleared = await clearPrivateDataInternal(clearables)
    clearDataCallback(false, isHistoryCleared)
  }

  private func clearPrivateDataInternal(_ clearables: [Clearable]) async -> Bool {
    @Sendable func _clear(_ clearables: [Clearable], secondAttempt: Bool = false) async {
      await withThrowingTaskGroup(of: Void.self) { group in
        for clearable in clearables {
          group.addTask {
            try await clearable.clear()
          }
        }
        do {
          for try await _ in group {}
        } catch {
          if !secondAttempt {
            Logger.module.error("Private data NOT cleared successfully")
            try? await Task.sleep(nanoseconds: NSEC_PER_MSEC * 500)
            await _clear(clearables, secondAttempt: true)
          } else {
            Logger.module.error("Private data NOT cleared after 2 attempts")
          }
        }
      }
    }

    let clearAffectsTabs = clearables.contains { item in
      return item is CacheClearable || item is CookiesAndCacheClearable
    }

    let historyCleared = clearables.contains { $0 is HistoryClearable }

    if clearAffectsTabs {
      DispatchQueue.main.async {
        self.tabManager.allTabs.forEach({ $0.reload() })
      }
    }

    @Sendable func _toggleFolderAccessForBlockCookies(locked: Bool) async {
      do {
        if Preferences.Privacy.blockAllCookies.value,
          try await AsyncFileManager.default.isWebDataLocked(atPath: .cookie) != locked
        {
          try await AsyncFileManager.default.setWebDataAccess(atPath: .cookie, lock: locked)
          try await AsyncFileManager.default.setWebDataAccess(atPath: .websiteData, lock: locked)
        }
      } catch {
        Logger.module.error("Failed to change web data access to \(locked)")
      }
    }

    try? await Task.sleep(nanoseconds: NSEC_PER_SEC * 1)

    // Reset Webkit configuration to remove data from memory
    if clearAffectsTabs {
      self.tabManager.reset()
      // This will recreate the webview for the selected tab.
      // Other tabs will have webviews re-created when they are selected
      self.tabManager.reloadSelectedTab()
      // Unlock the folders to allow clearing of data.
      await _toggleFolderAccessForBlockCookies(locked: false)
    }

    await _clear(clearables)
    if clearAffectsTabs {
      self.tabManager.allTabs.forEach({ $0.reload() })
    }

    if historyCleared {
      self.tabManager.clearTabHistory()

      // Clearing Tab History should clear Recently Closed
      RecentlyClosed.removeAll()
    }

    await _toggleFolderAccessForBlockCookies(locked: true)

    return historyCleared
  }

  private func registerSubscriptions() {
    FilterListStorage.shared.$filterLists
      .receive(on: DispatchQueue.main)
      .sink { filterLists in
        for filterList in filterLists {
          switch filterList.entry.componentId {
          case AdblockFilterListCatalogEntry.mobileAnnoyancesComponentID:
            if filterList.isEnabled != self.blockMobileAnnoyances {
              self.blockMobileAnnoyances = filterList.isEnabled
            }
          default:
            continue
          }
        }
      }
      .store(in: &subscriptions)
  }
}
