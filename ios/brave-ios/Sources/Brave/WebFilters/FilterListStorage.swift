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

  /// Wether or not these settings are stored in memory or persisted
  let persistChanges: Bool

  /// The filter list subscription
  private var filterListSubscription: AnyCancellable?
  /// A list of defaults that should be set once we load the filter lists.
  /// This is here in case the filter lists are not loaded but the user is already changing settings
  private var pendingDefaults: [String: Bool] = [:]
  /// The filter list subscription
  private var subscriptions: [AnyCancellable] = []
  /// Wether or not we loaded the catalog
  private var loadedCatalog = false
  /// We need this service to update settings
  private weak var adBlockService: AdblockService?

  /// The filter lists wrapped up so we can contain
  @Published var filterLists: [FilterList] = []

  /// This is a list of all available settings.
  ///
  /// - Warning: Do not call this before we load core data
  private(set) var allFilterListSettings: [FilterListSetting]

  init(persistChanges: Bool) {
    self.persistChanges = persistChanges
    allFilterListSettings = []
  }

  /// Will start listening to catalog changes
  func start(with adBlockService: AdblockService) {
    self.adBlockService = adBlockService
    loadFilterListSettings()

    Task { @MainActor in
      for await entries in adBlockService.filterListCatalogComponentStream() {
        loadFilterLists(from: entries, adBlockService: adBlockService)
        loadedCatalog = true
      }
    }
  }

  func updateFilterLists() async -> Bool {
    return await withCheckedContinuation { continuation in
      guard let adBlockService else {
        continuation.resume(returning: false)
        return
      }

      adBlockService.updateFilterLists({ updated in
        guard updated else {
          continuation.resume(returning: updated)
          return
        }
        for engineType in GroupedAdBlockEngine.EngineType.allCases {
          let fileInfos = adBlockService.fileInfos(for: engineType)
          AdBlockGroupsManager.shared.update(fileInfos: fileInfos)
        }
        continuation.resume(returning: updated)
      })
    }
  }

  /// Load the filter list settings
  private func loadFilterListSettings() {
    allFilterListSettings = FilterListSetting.loadAllSettings(fromMemory: !persistChanges)
  }

  /// Load filter lists from the ad block service and subscribe to any filter list changes
  /// - Warning: You should always call `loadFilterListSettings` before calling this
  private func loadFilterLists(
    from regionalFilterLists: [AdblockFilterListCatalogEntry],
    adBlockService: AdblockService
  ) {
    var filterLists = regionalFilterLists.enumerated().compactMap {
      index,
      entry -> FilterList? in
      let setting = allFilterListSettings.first(where: {
        $0.componentId == entry.componentId
      })
      // Some special filter lists don't have specific UI to disable it
      // (except for disabling all of ad-blocking)
      // For example the "default" and "first-party" list is controlled using our general Ad-block and TP toggle.
      let isEnabled =
        entry.hidden
        ? entry.defaultEnabled
        : setting?.isEnabled

      return FilterList(
        from: entry,
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

    // Set any pending defaults
    for (componentId, isEnabled) in pendingDefaults {
      guard let index = filterLists.firstIndex(where: { $0.entry.componentId == componentId })
      else { continue }
      filterLists[index].isEnabled = isEnabled
    }
    pendingDefaults.removeAll()

    // Create missing filter lists
    for filterList in filterLists {
      upsert(filterList: filterList)
    }

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
          // Ensure the storage is up to date
          self.upsert(filterList: filterList)

          // Ensure the service is fetching the files
          self.adBlockService?.enableFilterList(
            forUUID: filterList.entry.uuid,
            isEnabled: filterList.isEnabled
          )
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
      isAlwaysAggressive: filterList.engineType.isAlwaysAggressive,
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
  @MainActor var enabledSources: [GroupedAdBlockEngine.Source] {
    return filterLists.isEmpty
      ? allFilterListSettings
        .filter(\.isEnabled)
        .sorted(by: { $0.order?.intValue ?? 0 <= $1.order?.intValue ?? 0 })
        .compactMap(\.engineSource)
      : filterLists
        .filter(\.isEnabled)
        .map(\.engineSource)
  }

  func sources(
    for engineType: GroupedAdBlockEngine.EngineType
  ) -> [GroupedAdBlockEngine.Source] {
    return filterLists.isEmpty
      ? allFilterListSettings
        .filter({ $0.engineType == engineType })
        .sorted(by: { $0.order?.intValue ?? 0 <= $1.order?.intValue ?? 0 })
        .compactMap(\.engineSource)
      : filterLists
        .filter({ $0.engineType == engineType })
        .map(\.engineSource)
  }
}

extension AdblockService {
  @MainActor fileprivate func filterListCatalogComponentStream() -> AsyncStream<
    [AdblockFilterListCatalogEntry]
  > {
    return AsyncStream { continuation in
      registerCatalogChanges {
        continuation.yield(self.filterListCatalogEntries)
      }
    }
  }

  @MainActor fileprivate func filterListChangesStream() -> AsyncStream<
    GroupedAdBlockEngine.EngineType
  > {
    return AsyncStream { continuation in
      registerFilterListChanges({ isDefaultFilterList in
        continuation.yield(isDefaultFilterList ? .standard : .aggressive)
      })
    }
  }

  /// Get a list of files for the given engine type if the path exists
  @MainActor func fileInfos(
    for engineType: GroupedAdBlockEngine.EngineType
  ) -> [AdBlockEngineManager.FileInfo] {
    return filterListCatalogEntries.compactMap { entry in
      let source = entry.engineSource
      guard entry.engineType == engineType || source.onlyExceptions(for: engineType) else {
        return nil
      }

      guard
        let localFileURL = installPath(forFilterListUUID: entry.uuid),
        FileManager.default.fileExists(atPath: localFileURL.relativePath)
      else {
        return nil
      }

      return entry.fileInfo(for: localFileURL)
    }
  }
}

extension FilterListSetting {
  @MainActor var engineType: GroupedAdBlockEngine.EngineType {
    return isAlwaysAggressive ? .aggressive : .standard
  }
}
