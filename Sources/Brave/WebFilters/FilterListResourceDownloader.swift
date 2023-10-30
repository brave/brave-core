// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine
import Data
import BraveCore
import Shared
import Preferences
import BraveShields
import os.log

/// An object responsible for handling filer lists changes on `FilterListStorage` and registering them with the `AdBlockService`.
public actor FilterListResourceDownloader {
  /// A shared instance of this class
  ///
  /// - Warning: You need to wait for `DataController.shared.initializeOnce()` to be called before using this instance
  public static let shared = FilterListResourceDownloader()
  
  /// Object responsible for getting component updates
  private var adBlockService: AdblockService?
  /// Ad block service tasks per filter list UUID
  private var adBlockServiceTasks: [String: Task<Void, Error>]
  /// A marker that says that we loaded shield components for the first time.
  /// This boolean is used to configure this downloader only once after `AdBlockService` generic shields have been loaded.
  private var registeredFilterLists = false
  /// The path to the resources file
  private(set) var resourcesInfo: CachedAdBlockEngine.ResourcesInfo?
  
  init() {
    self.adBlockServiceTasks = [:]
    self.adBlockService = nil
  }
  
  /// This loads the filter list settings from core data.
  /// It uses these settings and other stored properties to load the enabled general shields and filter lists.
  ///
  /// - Warning: This method loads filter list settings.
  /// You need to wait for `DataController.shared.initializeOnce()` to be called first before invoking this method
  public func loadFilterListSettingsAndCachedData() async {
    if let defaultFilterListFolderURL = await FilterListSetting.makeFolderURL(
      forFilterListFolderPath: Preferences.AppState.lastFilterListCatalogueComponentFolderPath.value
    ), FileManager.default.fileExists(atPath: defaultFilterListFolderURL.path),
       let resourcesFolderURL = await FilterListSetting.makeFolderURL(
         forFilterListFolderPath: Preferences.AppState.lastAdBlockResourcesFolderPath.value
       ), FileManager.default.fileExists(atPath: resourcesFolderURL.path) {
      let resourcesInfo = await didUpdateResourcesComponent(folderURL: resourcesFolderURL)
      async let startedCustomFilterListsDownloader: Void = FilterListCustomURLDownloader.shared.startIfNeeded()
      async let cachedFilterLists: Void = compileCachedFilterLists(resourcesInfo: resourcesInfo)
      async let compileDefaultEngine: Void = compileDefaultEngine(defaultFilterListFolderURL: defaultFilterListFolderURL, resourcesInfo: resourcesInfo)
      _ = await (startedCustomFilterListsDownloader, cachedFilterLists, compileDefaultEngine)
    } else if let legacyComponentFolderURL = await FilterListSetting.makeFolderURL(
      forFilterListFolderPath: Preferences.AppState.lastLegacyDefaultFilterListFolderPath.value
    ), FileManager.default.fileExists(atPath: legacyComponentFolderURL.path) {
      // TODO: @JS Remove this after this release. Its here just so users can upgrade without a pause to their adblocking
      let resourcesInfo = await didUpdateResourcesComponent(folderURL: legacyComponentFolderURL)
      async let startedCustomFilterListsDownloader: Void = FilterListCustomURLDownloader.shared.startIfNeeded()
      async let cachedFilterLists: Void = compileCachedFilterLists(resourcesInfo: resourcesInfo)
      async let compileDefaultEngine: Void = compileDefaultEngine(defaultFilterListFolderURL: legacyComponentFolderURL, resourcesInfo: resourcesInfo)
      _ = await (startedCustomFilterListsDownloader, cachedFilterLists, compileDefaultEngine)
    }
  }
  
  /// This function adds engine resources to `AdBlockManager` from cached data representing the enabled filter lists.
  ///
  /// The filter lists are additional blocking content that can be added to the`AdBlockEngine` and as iOS content blockers.
  /// It represents the items found in the "Filter lists" section of the "Shields" menu.
  ///
  /// - Note: The content blockers for these filter lists are not loaded at this point. They are cached by iOS and there is no need to reload them.
  ///
  /// - Warning: This method loads filter list settings.
  /// You need to wait for `DataController.shared.initializeOnce()` to be called first before invoking this method
  private func compileCachedFilterLists(resourcesInfo: CachedAdBlockEngine.ResourcesInfo) async {
    await FilterListStorage.shared.loadFilterListSettings()
    let filterListSettings = await FilterListStorage.shared.allFilterListSettings
      
    do {
      try await filterListSettings.asyncConcurrentForEach { setting in
        guard await setting.isEnabled == true else { return }
        guard let componentId = await setting.componentId else { return }
        guard FilterList.disabledComponentIDs.contains(componentId) else { return }
        
        // Try to load the filter list folder. We always have to compile this at start
        guard let folderURL = await setting.folderURL, FileManager.default.fileExists(atPath: folderURL.path) else {
          return
        }
        
        await self.compileFilterListEngineIfNeeded(
          fromComponentId: componentId, folderURL: folderURL,
          isAlwaysAggressive: setting.isAlwaysAggressive,
          resourcesInfo: resourcesInfo,
          compileContentBlockers: false
        )
        
        // Sleep for 1ms. This drastically reduces memory usage without much impact to usability
        try await Task.sleep(nanoseconds: 1000000)
      }
    } catch {
      // Ignore the cancellation.
    }
  }
  
  /// Start the adblock service to get updates to the `shieldsInstallPath`
  public func start(with adBlockService: AdblockService) {
    self.adBlockService = adBlockService
    
    // Start listening to changes to the install url
    Task { @MainActor in
      for await folderURL in adBlockService.resourcesComponentStream() {
        guard let folderURL = folderURL else {
          ContentBlockerManager.log.error("Missing folder for filter lists")
          return
        }
        
        await didUpdateResourcesComponent(folderURL: folderURL)
        await FilterListCustomURLDownloader.shared.startIfNeeded()
        
        if !FilterListStorage.shared.filterLists.isEmpty {
          await registerAllFilterListsIfNeeded(with: adBlockService)
        }
      }
    }
    
    Task { @MainActor in
      for await filterListEntries in adBlockService.filterListCatalogComponentStream() {
        FilterListStorage.shared.loadFilterLists(from: filterListEntries)
        
        if await self.resourcesInfo != nil {
          await registerAllFilterListsIfNeeded(with: adBlockService)
        }
      }
    }
  }
  
  /// Register all enabled filter lists and to the default filter list with the `AdBlockService`
  private func registerAllFilterListsIfNeeded(with adBlockService: AdblockService) async {
    guard !registeredFilterLists else { return }
    self.registeredFilterLists = true
    registerToDefaultFilterList(with: adBlockService)
    
    for filterList in await FilterListStorage.shared.filterLists {
      register(filterList: filterList)
    }
  }
  
  /// Register to changes to the default filter list with the given ad-block service
  private func registerToDefaultFilterList(with adBlockService: AdblockService) {
    // Register the default filter list
    Task { @MainActor in
      for await folderURL in adBlockService.defaultComponentStream() {
        guard let folderURL = folderURL else {
          ContentBlockerManager.log.error("Missing folder for filter lists")
          return
        }
        
        await Task { @MainActor in
          let folderSubPath = FilterListSetting.extractFolderPath(fromFilterListFolderURL: folderURL)
          Preferences.AppState.lastFilterListCatalogueComponentFolderPath.value = folderSubPath
        }.value
        
        if let resourcesInfo = await self.resourcesInfo {
          await compileDefaultEngine(defaultFilterListFolderURL: folderURL, resourcesInfo: resourcesInfo)
        }
      }
    }
  }
  
  @discardableResult
  /// When the 
  private func didUpdateResourcesComponent(folderURL: URL) async -> CachedAdBlockEngine.ResourcesInfo {
    await Task { @MainActor in
      let folderSubPath = FilterListSetting.extractFolderPath(fromFilterListFolderURL: folderURL)
      Preferences.AppState.lastAdBlockResourcesFolderPath.value = folderSubPath
    }.value
    
    let version = folderURL.lastPathComponent
    let resourcesInfo = CachedAdBlockEngine.ResourcesInfo(
      localFileURL: folderURL.appendingPathComponent("resources.json", conformingTo: .json),
      version: version
    )
    
    self.resourcesInfo = resourcesInfo
    return resourcesInfo
  }
  
  /// Compile the general engine from the given `AdblockService` `shieldsInstallPath` `URL`.
  private func compileDefaultEngine(defaultFilterListFolderURL folderURL: URL, resourcesInfo: CachedAdBlockEngine.ResourcesInfo) async {
    // TODO: @JS Remove this on the next update. This is here so users don't have a pause to their ad-blocking
    let isLegacy = folderURL.pathExtension == "dat"
    let localFileURL = isLegacy ? folderURL.appendingPathComponent("rs-ABPFilterParserData.dat", conformingTo: .data) : folderURL.appendingPathComponent("list.txt", conformingTo: .text)
    
    let version = folderURL.lastPathComponent
    let filterListInfo = CachedAdBlockEngine.FilterListInfo(
      source: .adBlock,
      localFileURL: localFileURL,
      version: version, fileType: isLegacy ? .dat : .text
    )
    let lazyInfo = AdBlockStats.LazyFilterListInfo(
      filterListInfo: filterListInfo, isAlwaysAggressive: false
    )
    
    await AdBlockStats.shared.compile(
      lazyInfo: lazyInfo, resourcesInfo: resourcesInfo,
      compileContentBlockers: false
    )
  }
  
  /// Load general filter lists (shields) from the given `AdblockService` `shieldsInstallPath` `URL`.
  private func compileFilterListEngineIfNeeded(
    fromComponentId componentId: String, folderURL: URL,
    isAlwaysAggressive: Bool,
    resourcesInfo: CachedAdBlockEngine.ResourcesInfo,
    compileContentBlockers: Bool
  ) async {
    let version = folderURL.lastPathComponent
    let source = CachedAdBlockEngine.Source.filterList(componentId: componentId)
    let filterListInfo = CachedAdBlockEngine.FilterListInfo(
      source: source,
      localFileURL: folderURL.appendingPathComponent("list.txt", conformingTo: .text),
      version: version, fileType: .text
    )
    let lazyInfo = AdBlockStats.LazyFilterListInfo(filterListInfo: filterListInfo, isAlwaysAggressive: isAlwaysAggressive)
    guard await AdBlockStats.shared.isEagerlyLoaded(source: source) else {
      // Don't compile unless eager
      await AdBlockStats.shared.updateIfNeeded(resourcesInfo: resourcesInfo)
      await AdBlockStats.shared.updateIfNeeded(filterListInfo: filterListInfo, isAlwaysAggressive: isAlwaysAggressive)
      
      // To free some space, remove any rule lists that are not needed
      if let blocklistType = lazyInfo.blocklistType {
        do {
          try await ContentBlockerManager.shared.removeRuleLists(for: blocklistType)
        } catch {
          ContentBlockerManager.log.error("Failed to remove rule lists for \(filterListInfo.debugDescription)")
        }
      }
      return
    }
    
    await AdBlockStats.shared.compile(
      lazyInfo: lazyInfo, resourcesInfo: resourcesInfo,
      compileContentBlockers: compileContentBlockers
    )
  }
  
  /// Register this filter list with the `AdBlockService`
  private func register(filterList: FilterList) {
    guard adBlockServiceTasks[filterList.entry.componentId] == nil else { return }
    guard let adBlockService = adBlockService else { return }
    adBlockServiceTasks[filterList.entry.componentId]?.cancel()
    
    adBlockServiceTasks[filterList.entry.componentId] = Task { @MainActor in
      for await folderURL in adBlockService.register(filterList: filterList) {
        guard let folderURL = folderURL else { continue }
        guard let resourcesInfo = await self.resourcesInfo else {
          assertionFailure("We shouldn't have started downloads before getting this value")
          return
        }
        
        await self.loadShields(
          fromComponentId: filterList.entry.componentId, folderURL: folderURL, relativeOrder: filterList.order,
          loadContentBlockers: true,
          isAlwaysAggressive: filterList.isAlwaysAggressive,
          resourcesInfo: resourcesInfo
        )
        
        // Save the downloaded folder for later (caching) purposes
        FilterListStorage.shared.set(folderURL: folderURL, forUUID: filterList.entry.uuid)
      }
    }
  }
  
  /// Handle the downloaded component folder url of a filter list.
  ///
  /// The folder URL should point to a `AdblockFilterListEntry` download location as given by the `AdBlockService`.
  ///
  /// If `loadContentBlockers` is set to `true`, this method will compile the rule lists to content blocker format and load them into the `WKContentRuleListStore`.
  /// As both these procedures are expensive, this should be set to `false` if this method is called on a blocking UI process such as the launch of the application.
  private func loadShields(
    fromComponentId componentId: String, folderURL: URL, relativeOrder: Int, loadContentBlockers: Bool, isAlwaysAggressive: Bool,
    resourcesInfo: CachedAdBlockEngine.ResourcesInfo
  ) async {
    // Check if we're loading the new component or an old component from cache.
    // The new component has a file `list.txt` which we check the presence of.
    let filterListURL = folderURL.appendingPathComponent("list.txt", conformingTo: .text)
    
    guard FileManager.default.fileExists(atPath: filterListURL.relativePath) else {
      // We are loading the old component from cache. We don't want this file to be loaded.
      // When we download the new component shortly we will update our cache.
      // This should only trigger after an app update and eventually this check can be removed.
      return
    }
    
    // Add or remove the filter list from the engine depending if it's been enabled or not
    await self.compileFilterListEngineIfNeeded(
      fromComponentId: componentId, folderURL: folderURL, isAlwaysAggressive: isAlwaysAggressive,
      resourcesInfo: resourcesInfo, compileContentBlockers: loadContentBlockers
    )
  }
}

/// Helpful extension to the AdblockService
private extension AdblockService {
  @MainActor func defaultComponentStream() -> AsyncStream<URL?> {
    return AsyncStream { continuation in
      registerDefaultComponent { folderPath in
        guard let folderPath = folderPath else {
          continuation.yield(nil)
          return
        }
        
        let folderURL = URL(fileURLWithPath: folderPath)
        continuation.yield(folderURL)
      }
    }
  }
  
  @MainActor func resourcesComponentStream() -> AsyncStream<URL?> {
    return AsyncStream { continuation in
      registerResourceComponent { folderPath in
        guard let folderPath = folderPath else {
          continuation.yield(nil)
          return
        }
        
        let folderURL = URL(fileURLWithPath: folderPath)
        continuation.yield(folderURL)
      }
    }
  }
  
  @MainActor func filterListCatalogComponentStream() -> AsyncStream<[AdblockFilterListCatalogEntry]> {
    return AsyncStream { continuation in
      registerFilterListCatalogComponent { filterListEntries in
        continuation.yield(filterListEntries)
      }
    }
  }
  
  /// Register the filter list given by the uuid and streams its updates
  ///
  /// - Note: Cancelling this task will unregister this filter list from recieving any further updates
  @MainActor func register(filterList: FilterList) -> AsyncStream<URL?> {
    return AsyncStream { continuation in
      registerFilterListComponent(filterList.entry) { folderPath in
        guard let folderPath = folderPath else {
          continuation.yield(nil)
          return
        }
        
        let folderURL = URL(fileURLWithPath: folderPath)
        continuation.yield(folderURL)
      }
      
      continuation.onTermination = { @Sendable _ in
        self.unregisterFilterListComponent(filterList.entry)
      }
    }
  }
}
