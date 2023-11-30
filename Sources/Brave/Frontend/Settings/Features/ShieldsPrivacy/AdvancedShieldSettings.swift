// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine
import BraveCore
import BraveNews
import Preferences
import BraveShields
import Data
import Growth
import os

@MainActor class AdvancedShieldsSettings: ObservableObject {
  struct ClearableSetting: Identifiable {
    enum ClearableType: String {
      case history, cache, cookiesAndCache, passwords, downloads, braveNews, playlistCache, playlistData, recentSearches
    }
    
    var id: ClearableType
    var clearable: Clearable
    var isEnabled: Bool
  }
  
  @Published var cookieConsentBlocking: Bool {
    didSet {
      FilterListStorage.shared.ensureFilterList(
        for: FilterList.cookieConsentNoticesComponentID, isEnabled: cookieConsentBlocking
      )
    }
  }
  @Published var blockMobileAnnoyances: Bool {
    didSet {
      FilterListStorage.shared.ensureFilterList(
        for: FilterList.mobileAnnoyancesComponentID, isEnabled: blockMobileAnnoyances
      )
    }
  }
  @Published var isP3AEnabled: Bool {
    didSet {
      p3aUtilities.isP3AEnabled = isP3AEnabled
    }
  }
  @Published var adBlockAndTrackingPreventionLevel: ShieldLevel {
    didSet {
      ShieldPreferences.blockAdsAndTrackingLevel = adBlockAndTrackingPreventionLevel
      if adBlockAndTrackingPreventionLevel != oldValue {
        recordGlobalAdBlockShieldsP3A()
      }
    }
  }
  
  typealias ClearDataCallback = @MainActor (Bool, Bool) -> Void
  @Published var clearableSettings: [ClearableSetting]
  
  private var subscriptions: [AnyCancellable] = []
  private let p3aUtilities: BraveP3AUtils
  private let clearDataCallback: ClearDataCallback
  let tabManager: TabManager
  
  init(
    profile: Profile, tabManager: TabManager,
    feedDataSource: FeedDataSource, historyAPI: BraveHistoryAPI,
    p3aUtilities: BraveP3AUtils,
    clearDataCallback: @escaping ClearDataCallback
  ) {
    self.p3aUtilities = p3aUtilities
    self.tabManager = tabManager
    self.isP3AEnabled = p3aUtilities.isP3AEnabled
    self.clearDataCallback = clearDataCallback
    self.adBlockAndTrackingPreventionLevel = ShieldPreferences.blockAdsAndTrackingLevel
    
    cookieConsentBlocking = FilterListStorage.shared.isEnabled(
      for: FilterList.cookieConsentNoticesComponentID
    )
    
    blockMobileAnnoyances = FilterListStorage.shared.isEnabled(
      for: FilterList.mobileAnnoyancesComponentID
    )
    
    var clearableSettings = [
      ClearableSetting(id: .history, clearable: HistoryClearable(historyAPI: historyAPI), isEnabled: true),
      ClearableSetting(id: .cache, clearable: CacheClearable(), isEnabled: true),
      ClearableSetting(id: .cookiesAndCache, clearable: CookiesAndCacheClearable(), isEnabled: true),
      ClearableSetting(id: .passwords, clearable: PasswordsClearable(profile: profile), isEnabled: true),
      ClearableSetting(id: .downloads, clearable: DownloadsClearable(), isEnabled: true),
      ClearableSetting(id: .braveNews, clearable: BraveNewsClearable(feedDataSource: feedDataSource), isEnabled: true),
      ClearableSetting(id: .playlistCache, clearable: PlayListCacheClearable(), isEnabled: false),
      ClearableSetting(id: .playlistData, clearable: PlayListDataClearable(), isEnabled: false),
      ClearableSetting(id: .recentSearches, clearable: RecentSearchClearable(), isEnabled: true)
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
          for try await _ in group { }
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

    @Sendable func _toggleFolderAccessForBlockCookies(locked: Bool) {
      if Preferences.Privacy.blockAllCookies.value, FileManager.default.checkLockedStatus(folder: .cookie) != locked {
        FileManager.default.setFolderAccess([
          (.cookie, locked),
          (.webSiteData, locked),
        ])
      }
    }

    try? await Task.sleep(nanoseconds: NSEC_PER_SEC * 1)
    
    // Reset Webkit configuration to remove data from memory
    if clearAffectsTabs {
      self.tabManager.resetConfiguration()
      // Unlock the folders to allow clearing of data.
      _toggleFolderAccessForBlockCookies(locked: false)
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
    
    _toggleFolderAccessForBlockCookies(locked: true)
    
    return historyCleared
  }
  
  private func registerSubscriptions() {
    FilterListStorage.shared.$filterLists
      .receive(on: DispatchQueue.main)
      .sink { filterLists in
        for filterList in filterLists {
          switch filterList.entry.componentId {
          case FilterList.cookieConsentNoticesComponentID:
            if filterList.isEnabled != self.cookieConsentBlocking {
              self.cookieConsentBlocking = filterList.isEnabled
            }
          case FilterList.mobileAnnoyancesComponentID:
            if filterList.isEnabled != self.blockMobileAnnoyances {
              self.blockMobileAnnoyances = filterList.isEnabled
            }
          default:
            continue
          }
        }
      }
      .store(in: &subscriptions)
    
    Preferences.Shields.fingerprintingProtection.$value
      .sink { [weak self] _ in
        self?.recordGlobalFingerprintingShieldsP3A()
      }
      .store(in: &subscriptions)
  }
  
  // MARK: - P3A
  
  private func recordGlobalAdBlockShieldsP3A() {
    // Q46 What is the global ad blocking shields setting?
    enum Answer: Int, CaseIterable {
      case disabled = 0
      case standard = 1
      case aggressive = 2
    }
    
    let answer = { () -> Answer in
      switch ShieldPreferences.blockAdsAndTrackingLevel {
      case .disabled: return .disabled
      case .standard: return .standard
      case .aggressive: return .aggressive
      }
    }()
    
    UmaHistogramEnumeration("Brave.Shields.AdBlockSetting", sample: answer)
  }
  
  private func recordGlobalFingerprintingShieldsP3A() {
    // Q47 What is the global fingerprinting shields setting?
    enum Answer: Int, CaseIterable {
      case disabled = 0
      case standard = 1
      case aggressive = 2
    }
    let answer: Answer = Preferences.Shields.fingerprintingProtection.value ? .standard : .disabled
    UmaHistogramEnumeration("Brave.Shields.FingerprintBlockSetting", sample: answer)
  }
}
