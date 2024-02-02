// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Data
import Foundation
import Onboarding
import Preferences

@MainActor class FilterListStorage: ObservableObject {
  static let shared = FilterListStorage(persistChanges: true)

  /// A list of loaded versions for the filter lists with the componentId as the key and version as the value
  var loadedRuleListVersions = Preferences.Option<[String: String]>(
    key: "filter_list_resource_downloader.loaded-adblock-versions",
    default: [:]
  )

  /// Wether or not these settings are stored in memory or persisted
  let persistChanges: Bool

  /// The filter list subscription
  private var filterListSubscription: AnyCancellable?
  /// A list of defaults that should be set once we load the filter lists.
  /// This is here in case the filter lists are not loaded but the user is already changing settings
  private var pendingDefaults: [String: Bool] = [:]
  /// The filter list subscription
  private var subscriptions: [AnyCancellable] = []

  /// The filter lists wrapped up so we can contain
  @Published var filterLists: [FilterList] = []

  /// This is a list of all available settings.
  ///
  /// - Warning: Do not call this before we load core data
  private(set) var allFilterListSettings: [FilterListSetting]

  init(persistChanges: Bool) {
    self.persistChanges = persistChanges
    allFilterListSettings = []
    recordP3ACookieListEnabled()
  }

  /// Load the filter list settings
  func loadFilterListSettings() {
    allFilterListSettings = FilterListSetting.loadAllSettings(fromMemory: !persistChanges)
  }

  /// Load filter lists from the ad block service and subscribe to any filter list changes
  /// - Warning: You should always call `loadFilterListSettings` before calling this
  func loadFilterLists(from regionalFilterLists: [AdblockFilterListCatalogEntry]) {
    let filterLists = regionalFilterLists.enumerated().compactMap {
      index,
      adBlockFilterList -> FilterList? in
      // Certain filter lists are disabled if they are currently incompatible with iOS
      guard !FilterList.disabledComponentIDs.contains(adBlockFilterList.componentId) else {
        return nil
      }
      let setting = allFilterListSettings.first(where: {
        $0.componentId == adBlockFilterList.componentId
      })
      // Some special filter lists don't have specific UI to disable it
      // (except for disabling all of ad-blocking)
      // For example the "default" and "first-party" list is controlled using our general Ad-block and TP toggle.
      let isEnabled =
        adBlockFilterList.hidden
        ? adBlockFilterList.defaultEnabled
        : pendingDefaults[adBlockFilterList.componentId] ?? setting?.isEnabled

      return FilterList(
        from: adBlockFilterList,
        order: index,
        isEnabled: isEnabled
      )
    }

    // Delete dead settings
    for setting in allFilterListSettings
    where !regionalFilterLists.contains(where: { $0.componentId == setting.componentId }) {
      allFilterListSettings.removeAll(where: { $0.componentId == setting.componentId })
      setting.delete(inMemory: !persistChanges)
    }

    // Create missing filter lists
    for filterList in filterLists {
      upsert(filterList: filterList)
    }

    pendingDefaults.removeAll()
    FilterListSetting.save(inMemory: !persistChanges)
    self.filterLists = filterLists

    // Now that our filter lists are loaded, let's subscribe to any changes to them
    // This way we ensure our settings are always stored.
    subscribeToFilterListChanges()
  }

  /// Ensures that the settings for a filter list are stored
  /// - Parameters:
  ///   - componentId: The component id of the filter list to update
  ///   - isEnabled: A boolean indicating if the filter list is enabled or not
  public func ensureFilterList(for componentId: String, isEnabled: Bool) {
    defer { self.recordP3ACookieListEnabled() }

    // Enable the setting
    if let index = filterLists.firstIndex(where: { $0.entry.componentId == componentId }) {
      // Only update the value if it has changed
      guard filterLists[index].isEnabled != isEnabled else { return }
      filterLists[index].isEnabled = isEnabled
    } else if let index = allFilterListSettings.firstIndex(where: { $0.componentId == componentId })
    {
      // If we haven't loaded the filter lists yet, at least attempt to update the settings
      allFilterListSettings[index].isEnabled = isEnabled
      allFilterListSettings[index].componentId = componentId
      FilterListSetting.save(inMemory: !persistChanges)
    } else {
      // If we haven't even loaded the settings yet, set the pending defaults
      // This will force the creation of the setting once filter lists have been loaded
      pendingDefaults[componentId] = isEnabled
    }
  }

  /// - Warning: Do not call this before we load core data
  public func isEnabled(for componentId: String) -> Bool {
    guard !FilterList.disabledComponentIDs.contains(componentId) else { return false }

    return filterLists.first(where: { $0.entry.componentId == componentId })?.isEnabled
      ?? allFilterListSettings.first(where: { $0.componentId == componentId })?.isEnabled
      ?? pendingDefaults[componentId]
      ?? false
  }

  /// Subscribe to any filter list changes so our settings are always stored
  private func subscribeToFilterListChanges() {
    $filterLists
      .receive(on: DispatchQueue.main)
      .sink { filterLists in
        for filterList in filterLists {
          self.upsert(filterList: filterList)
        }
      }
      .store(in: &subscriptions)
  }

  /// Upsert (update or insert) the setting.
  private func upsert(filterList: FilterList) {
    upsertSetting(
      uuid: filterList.entry.uuid,
      isEnabled: filterList.isEnabled,
      isHidden: filterList.isHidden,
      componentId: filterList.entry.componentId,
      allowCreation: true,
      order: filterList.order,
      isAlwaysAggressive: filterList.isAlwaysAggressive,
      isDefaultEnabled: filterList.entry.defaultEnabled
    )
  }

  /// Set the enabled status and componentId of a filter list setting if the setting exists.
  /// Otherwise it will create a new setting with the specified properties
  ///
  /// - Warning: Do not call this before we load core data
  private func upsertSetting(
    uuid: String,
    isEnabled: Bool,
    isHidden: Bool,
    componentId: String,
    allowCreation: Bool,
    order: Int,
    isAlwaysAggressive: Bool,
    isDefaultEnabled: Bool
  ) {
    if allFilterListSettings.contains(where: { $0.uuid == uuid }) {
      updateSetting(
        uuid: uuid,
        componentId: componentId,
        isEnabled: isEnabled,
        isHidden: isHidden,
        order: order,
        isAlwaysAggressive: isAlwaysAggressive,
        isDefaultEnabled: isDefaultEnabled
      )
    } else if allowCreation {
      create(
        uuid: uuid,
        componentId: componentId,
        isEnabled: isEnabled,
        isHidden: isHidden,
        order: order,
        isAlwaysAggressive: isAlwaysAggressive,
        isDefaultEnabled: isDefaultEnabled
      )
    }
  }

  /// Set the folder url of the
  ///
  /// - Warning: Do not call this before we load core data
  public func set(folderURL: URL, forUUID uuid: String) {
    guard let index = allFilterListSettings.firstIndex(where: { $0.uuid == uuid }) else {
      return
    }

    guard allFilterListSettings[index].folderURL != folderURL else { return }
    allFilterListSettings[index].folderURL = folderURL
    FilterListSetting.save(inMemory: !persistChanges)
  }

  /// Update the filter list settings with the given `componentId` and `isEnabled` status
  /// Will not write unless one of these two values have changed
  private func updateSetting(
    uuid: String,
    componentId: String,
    isEnabled: Bool,
    isHidden: Bool,
    order: Int,
    isAlwaysAggressive: Bool,
    isDefaultEnabled: Bool
  ) {
    guard let index = allFilterListSettings.firstIndex(where: { $0.uuid == uuid }) else {
      return
    }

    // Ensure we stop if this is already in sync in order to avoid an event loop
    // And things hanging for too long.
    guard
      allFilterListSettings[index].isEnabled != isEnabled
        || allFilterListSettings[index].componentId != componentId
        || allFilterListSettings[index].order?.intValue != order
        || allFilterListSettings[index].isAlwaysAggressive != isAlwaysAggressive
        || allFilterListSettings[index].isHidden != isHidden
        || allFilterListSettings[index].isDefaultEnabled != isDefaultEnabled
    else {
      return
    }

    allFilterListSettings[index].isEnabled = isEnabled
    allFilterListSettings[index].isAlwaysAggressive = isAlwaysAggressive
    allFilterListSettings[index].isHidden = isHidden
    allFilterListSettings[index].componentId = componentId
    allFilterListSettings[index].order = NSNumber(value: order)
    allFilterListSettings[index].isDefaultEnabled = isDefaultEnabled
    FilterListSetting.save(inMemory: !persistChanges)
  }

  /// Create a filter list setting for the given UUID and enabled status
  private func create(
    uuid: String,
    componentId: String,
    isEnabled: Bool,
    isHidden: Bool,
    order: Int,
    isAlwaysAggressive: Bool,
    isDefaultEnabled: Bool
  ) {
    let setting = FilterListSetting.create(
      uuid: uuid,
      componentId: componentId,
      isEnabled: isEnabled,
      isHidden: isHidden,
      order: order,
      inMemory: !persistChanges,
      isAlwaysAggressive: isAlwaysAggressive,
      isDefaultEnabled: isDefaultEnabled
    )
    allFilterListSettings.append(setting)
  }

  // MARK: - P3A

  private func recordP3ACookieListEnabled() {
    // Q69 Do you have cookie consent notice blocking enabled?
    Task { @MainActor in
      UmaHistogramBoolean(
        "Brave.Shields.CookieListEnabled",
        isEnabled(for: FilterList.cookieConsentNoticesComponentID)
      )
    }
  }
}

// MARK: - FilterListLanguageProvider - A way to share `defaultToggle` logic between multiple structs/classes

extension AdblockFilterListCatalogEntry {
  @available(iOS 16, *)
  /// A list of regions that this filter list focuses on.
  /// An empty set means this filter list doesn't focus on any specific region.
  fileprivate var supportedLanguageCodes: Set<Locale.LanguageCode> {
    return Set(languages.map({ Locale.LanguageCode($0) }))
  }
}

extension FilterListStorage {
  /// Gives us source representations of all the enabled filter lists
  @MainActor var enabledSources: [CachedAdBlockEngine.Source] {
    if !filterLists.isEmpty {
      return filterLists.compactMap { filterList -> CachedAdBlockEngine.Source? in
        guard filterList.isEnabled else { return nil }
        return filterList.engineSource
      }
    } else {
      // We may not have the filter lists loaded yet. In which case we load the settings
      return allFilterListSettings.compactMap { setting -> CachedAdBlockEngine.Source? in
        guard setting.isEnabled else { return nil }
        return setting.engineSource
      }
    }
  }
}
