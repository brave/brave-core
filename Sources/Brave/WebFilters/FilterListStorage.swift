// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import BraveCore

@MainActor class FilterListStorage: ObservableObject {
  static let shared = FilterListStorage(persistChanges: true)
  
  /// Wether or not these settings are stored in memory or persisted
  let persistChanges: Bool
  
  /// A list of defaults that should be set once we load the filter lists.
  /// This is here in case the filter lists are not loaded but the user is already changing settings
  private var pendingDefaults: [String: Bool] = [:]
  
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
  
  func loadFilterListSettings() {
    allFilterListSettings = FilterListSetting.loadAllSettings(fromMemory: !persistChanges)
  }
  
  /// Load filter lists from the ad block service
  /// - Warning: You should always call `loadFilterListSettings` before calling this
  func loadFilterLists(from regionalFilterLists: [AdblockFilterListCatalogEntry]) {
    let filterLists = regionalFilterLists.enumerated().compactMap { index, adBlockFilterList -> FilterList? in
      // Certain filter lists are disabled if they are currently incompatible with iOS
      guard !FilterList.disabledComponentIDs.contains(adBlockFilterList.componentId) else { return nil }
      let setting = allFilterListSettings.first(where: { $0.componentId == adBlockFilterList.componentId })
      
      return FilterList(
        from: adBlockFilterList,
        order: index,
        isEnabled: setting?.isEnabled ?? pendingDefaults[adBlockFilterList.componentId] ?? adBlockFilterList.defaultToggle
      )
    }
    
    // Delete dead settings
    for setting in allFilterListSettings where !regionalFilterLists.contains(where: { $0.componentId == setting.componentId }) {
      allFilterListSettings.removeAll(where: { $0.componentId == setting.componentId })
      setting.delete(inMemory: !persistChanges)
    }
    
    // Save pending toggles
    for (componentId, isEnabled) in pendingDefaults {
      if let index = allFilterListSettings.firstIndex(where: { $0.componentId == componentId }) {
        allFilterListSettings[index].isEnabled = isEnabled
      }
    }
    
    FilterListSetting.save(inMemory: !persistChanges)
    pendingDefaults.removeAll()
    self.filterLists = filterLists
  }
  
  /// Enables a filter list for the given component ID. Returns true if the filter list exists or not.
  public func enableFilterList(for componentId: String, isEnabled: Bool) {
    defer { self.recordP3ACookieListEnabled() }
    
    // Enable the setting
    if let index = filterLists.firstIndex(where: { $0.entry.componentId == componentId }) {
      // Only update the value if it has changed
      guard filterLists[index].isEnabled != isEnabled else { return }
      filterLists[index].isEnabled = isEnabled
    } else if let index = allFilterListSettings.firstIndex(where: { $0.componentId == componentId }) {
      allFilterListSettings[index].isEnabled = isEnabled
      allFilterListSettings[index].componentId = componentId
      FilterListSetting.save(inMemory: !persistChanges)
    } else {
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
  
  /// Upsert (update or insert) the setting.
  ///
  /// However we create only
  /// 1. When the filter list is different than the default
  ///  (In order to respect the users preference if the default were to change in the future)
  /// 2. When the filter list is enabled:
  ///  (So we can load the setting from the cache during launch)
  func handleUpdate(to filterList: FilterList) {
    upsertSetting(
      uuid: filterList.entry.uuid,
      isEnabled: filterList.isEnabled,
      componentId: filterList.entry.componentId,
      allowCreation: filterList.entry.defaultToggle != filterList.isEnabled || filterList.isEnabled,
      order: filterList.order,
      isAlwaysAggressive: filterList.isAlwaysAggressive
    )
  }
  
  /// Set the enabled status and componentId of a filter list setting if the setting exists.
  /// Otherwise it will create a new setting with the specified properties
  ///
  /// - Warning: Do not call this before we load core data
  private func upsertSetting(
    uuid: String, isEnabled: Bool, componentId: String,
    allowCreation: Bool, order: Int, isAlwaysAggressive: Bool
  ) {
    if allFilterListSettings.contains(where: { $0.uuid == uuid }) {
      updateSetting(
        uuid: uuid,
        componentId: componentId,
        isEnabled: isEnabled,
        order: order,
        isAlwaysAggressive: isAlwaysAggressive
      )
    } else if allowCreation {
      create(
        uuid: uuid,
        componentId: componentId,
        isEnabled: isEnabled,
        order: order,
        isAlwaysAggressive: isAlwaysAggressive
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
  private func updateSetting(uuid: String, componentId: String, isEnabled: Bool, order: Int, isAlwaysAggressive: Bool) {
    guard let index = allFilterListSettings.firstIndex(where: { $0.uuid == uuid }) else {
      return
    }
    
    guard allFilterListSettings[index].isEnabled != isEnabled || allFilterListSettings[index].componentId != componentId || allFilterListSettings[index].order?.intValue != order || allFilterListSettings[index].isAlwaysAggressive != isAlwaysAggressive else {
      // Ensure we stop if this is already in sync in order to avoid an event loop
      // And things hanging for too long.
      // This happens because we care about UI changes but not when our downloads finish
      return
    }
      
    allFilterListSettings[index].isEnabled = isEnabled
    allFilterListSettings[index].isAlwaysAggressive = isAlwaysAggressive
    allFilterListSettings[index].componentId = componentId
    allFilterListSettings[index].order = NSNumber(value: order)
    FilterListSetting.save(inMemory: !persistChanges)
  }
  
  /// Create a filter list setting for the given UUID and enabled status
  private func create(uuid: String, componentId: String, isEnabled: Bool, order: Int, isAlwaysAggressive: Bool) {
    let setting = FilterListSetting.create(
      uuid: uuid, componentId: componentId, isEnabled: isEnabled, order: order, inMemory: !persistChanges,
      isAlwaysAggressive: isAlwaysAggressive
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

private extension AdblockFilterListCatalogEntry {
  @available(iOS 16, *)
  /// A list of regions that this filter list focuses on.
  /// An empty set means this filter list doesn't focus on any specific region.
  var supportedLanguageCodes: Set<Locale.LanguageCode> {
    return Set(languages.map({ Locale.LanguageCode($0) }))
  }
  
  /// This method returns the default value for this filter list if the user does not manually toggle it.
  /// - Warning: Make sure you use `componentID` to identify the filter list, as `uuid` will be deprecated in the future.
  var defaultToggle: Bool {
    let componentIDsToOverride = [FilterList.mobileAnnoyancesComponentID]
    
    if componentIDsToOverride.contains(componentId) {
      return true
    }
    
    // For compatibility reasons, we only enable certian regional filter lists
    // These are the ones that are known to be well maintained.
    guard FilterList.maintainedRegionalComponentIDs.contains(componentId) else {
      return false
    }
    
    if #available(iOS 16, *), let languageCode = Locale.current.language.languageCode {
      return supportedLanguageCodes.contains(languageCode)
    } else if let languageCode = Locale.current.languageCode {
      return languages.contains(languageCode)
    } else {
      return false
    }
  }
}
