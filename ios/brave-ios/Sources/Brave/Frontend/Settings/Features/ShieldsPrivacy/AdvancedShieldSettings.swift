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

  @Published var cookieConsentBlocking: Bool {
    didSet {
      FilterListStorage.shared.ensureFilterList(
        for: AdblockFilterListCatalogEntry.cookieConsentNoticesComponentID,
        isEnabled: cookieConsentBlocking
      )
    }
  }
  @Published var blockMobileAnnoyances: Bool {
    didSet {
      FilterListStorage.shared.ensureFilterList(
        for: AdblockFilterListCatalogEntry.mobileAnnoyancesComponentID,
        isEnabled: blockMobileAnnoyances
      )
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
      ShieldPreferences.blockAdsAndTrackingLevel = adBlockAndTrackingPreventionLevel
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
      ShieldPreferences.shredLevel = shredLevel
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

  typealias ClearDataCallback = @MainActor (Bool, Bool) -> Void
  @Published var clearableSettings: [ClearableSetting]

  private var subscriptions: [AnyCancellable] = []
  private let p3aUtilities: BraveP3AUtils
  private let deAmpPrefs: DeAmpPrefs
  private let debounceService: DebounceService?
  private let rewards: BraveRewards?
  private let clearDataCallback: ClearDataCallback
  private let webcompatReporterHandler: WebcompatReporterWebcompatReporterHandler?
  let tabManager: TabManager

  init(
    profile: Profile,
    tabManager: TabManager,
    feedDataSource: FeedDataSource,
    debounceService: DebounceService?,
    braveCore: BraveCoreMain,
    rewards: BraveRewards?,
    webcompatReporterHandler: WebcompatReporterWebcompatReporterHandler?,
    clearDataCallback: @escaping ClearDataCallback
  ) {
    self.p3aUtilities = braveCore.p3aUtils
    self.deAmpPrefs = braveCore.deAmpPrefs
    self.debounceService = debounceService
    self.tabManager = tabManager
    self.isP3AEnabled = p3aUtilities.isP3AEnabled
    self.rewards = rewards
    self.clearDataCallback = clearDataCallback
    self.adBlockAndTrackingPreventionLevel = ShieldPreferences.blockAdsAndTrackingLevel
    self.httpsUpgradeLevel = ShieldPreferences.httpsUpgradeLevel
    self.isDeAmpEnabled = deAmpPrefs.isDeAmpEnabled
    self.isDebounceEnabled = debounceService?.isEnabled ?? false
    self.shredLevel = ShieldPreferences.shredLevel
    self.webcompatReporterHandler = webcompatReporterHandler

    cookieConsentBlocking = FilterListStorage.shared.isEnabled(
      for: AdblockFilterListCatalogEntry.cookieConsentNoticesComponentID
    )

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
      ClearableSetting(
        id: .braveNews,
        clearable: BraveNewsClearable(feedDataSource: feedDataSource),
        isEnabled: true
      ),
    ]

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

    clearableSettings += [
      ClearableSetting(id: .playlistCache, clearable: PlayListCacheClearable(), isEnabled: false),
      ClearableSetting(id: .playlistData, clearable: PlayListDataClearable(), isEnabled: false),
      ClearableSetting(id: .recentSearches, clearable: RecentSearchClearable(), isEnabled: true),
    ]

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
      self.isSaveContactInfoEnabled = await webcompatReporterHandler?.contactInfoSaveFlag() ?? false
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
      self.tabManager.resetConfiguration()
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
          case AdblockFilterListCatalogEntry.cookieConsentNoticesComponentID:
            if filterList.isEnabled != self.cookieConsentBlocking {
              self.cookieConsentBlocking = filterList.isEnabled
            }
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
