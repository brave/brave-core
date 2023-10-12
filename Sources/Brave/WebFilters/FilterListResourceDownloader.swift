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
  private var loadedShieldComponents = false
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
    guard let folderURL = await FilterListSetting.makeFolderURL(
      forFilterListFolderPath: Preferences.AppState.lastDefaultFilterListFolderPath.value
    ), FileManager.default.fileExists(atPath: folderURL.path) else {
      return
    }
    
    let version = folderURL.lastPathComponent
    let resourcesInfo = CachedAdBlockEngine.ResourcesInfo(
      localFileURL: folderURL.appendingPathComponent("resources.json", conformingTo: .json),
      version: version
    )
    self.resourcesInfo = resourcesInfo
    
    async let startedCustomFilterListsDownloader: Void = FilterListCustomURLDownloader.shared.start()
    async let cachedFilterLists: Void = compileCachedFilterLists(resourcesInfo: resourcesInfo)
    async let compileDefaultEngine: Void = compileDefaultEngine(shieldsInstallFolder: folderURL, resourcesInfo: resourcesInfo)
    _ = await (startedCustomFilterListsDownloader, cachedFilterLists, compileDefaultEngine)
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
          resourcesInfo: resourcesInfo
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
      for await folderURL in adBlockService.shieldsInstallURL {
        await self.didUpdateShieldComponent(
          folderURL: folderURL,
          adBlockFilterLists: adBlockService.regionalFilterLists ?? []
        )
      }
    }
  }
  
  /// Invoked when shield components are loaded
  ///
  /// This function will start fetching data and subscribe publishers once if it hasn't already done so.
  private func didUpdateShieldComponent(folderURL: URL, adBlockFilterLists: [AdblockFilterListCatalogEntry]) async {
    // Store the folder path so we can load it from cache next time we launch quicker
    // than waiting for the component updater to respond, which may take a few seconds
    await Task { @MainActor in
      let folderSubPath = FilterListSetting.extractFolderPath(fromFilterListFolderURL: folderURL)
      Preferences.AppState.lastDefaultFilterListFolderPath.value = folderSubPath
    }.value
    
    // Set the resources info so other filter lists can use them
    let version = folderURL.lastPathComponent
    let resourcesInfo = CachedAdBlockEngine.ResourcesInfo(
      localFileURL: folderURL.appendingPathComponent("resources.json", conformingTo: .json),
      version: version
    )
    self.resourcesInfo = resourcesInfo
    
    // Perform one time setup
    if !loadedShieldComponents && !adBlockFilterLists.isEmpty {
      // This is the first time we load ad-block filters.
      // We need to perform some initial setup (but only do this once)
      loadedShieldComponents = true
      await FilterListStorage.shared.loadFilterLists(from: adBlockFilterLists)
      
      Task {
        // Start the custom filter list downloader
        await FilterListCustomURLDownloader.shared.start()
      }
    }
  
    // Compile the engine
    await compileDefaultEngine(shieldsInstallFolder: folderURL, resourcesInfo: resourcesInfo)
    await registerAllFilterLists()
  }
  
  /// Compile the general engine from the given `AdblockService` `shieldsInstallPath` `URL`.
  private func compileDefaultEngine(shieldsInstallFolder folderURL: URL, resourcesInfo: CachedAdBlockEngine.ResourcesInfo) async {
    let version = folderURL.lastPathComponent
    let filterListInfo = CachedAdBlockEngine.FilterListInfo(
      source: .adBlock,
      localFileURL: folderURL.appendingPathComponent("rs-ABPFilterParserData.dat", conformingTo: .data),
      version: version, fileType: .dat
    )
    
    guard await AdBlockStats.shared.needsCompilation(for: filterListInfo, resourcesInfo: resourcesInfo) else {
      return
    }
    
    await AdBlockStats.shared.compile(
      filterListInfo: filterListInfo, resourcesInfo: resourcesInfo,
      isAlwaysAggressive: false
    )
  }
  
  /// Load general filter lists (shields) from the given `AdblockService` `shieldsInstallPath` `URL`.
  private func compileFilterListEngineIfNeeded(
    fromComponentId componentId: String, folderURL: URL,
    isAlwaysAggressive: Bool,
    resourcesInfo: CachedAdBlockEngine.ResourcesInfo
  ) async {
    let version = folderURL.lastPathComponent
    let source = CachedAdBlockEngine.Source.filterList(componentId: componentId)
    let filterListInfo = CachedAdBlockEngine.FilterListInfo(
      source: source,
      localFileURL: folderURL.appendingPathComponent("list.txt", conformingTo: .text),
      version: version, fileType: .text
    )
    
    guard await AdBlockStats.shared.isEagerlyLoaded(source: source) else {
      // Don't compile unless eager
      await AdBlockStats.shared.updateIfNeeded(resourcesInfo: resourcesInfo)
      await AdBlockStats.shared.updateIfNeeded(filterListInfo: filterListInfo, isAlwaysAggressive: isAlwaysAggressive)
      return
    }
    
    guard await AdBlockStats.shared.needsCompilation(for: filterListInfo, resourcesInfo: resourcesInfo) else {
      // Don't compile unless needed
      return
    }
    
    let isImportant = await AdBlockStats.shared.criticalSources.contains(source)
     
    do {
      try await AdBlockStats.shared.compileDelayed(
        filterListInfo: filterListInfo, resourcesInfo: resourcesInfo,
        isAlwaysAggressive: isAlwaysAggressive, delayed: !isImportant
      )
    } catch {
      // Don't handle cancellation errors
    }
  }
  
  /// Register all enabled filter lists with the `AdBlockService`
  @MainActor private func registerAllFilterLists() async {
    for filterList in FilterListStorage.shared.filterLists {
      await register(filterList: filterList)
    }
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
        FilterListStorage.shared.set(folderURL: folderURL, forUUID: filterList.uuid)
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
      resourcesInfo: resourcesInfo
    )
    
    // Compile this rule list if we haven't already or if the file has been modified
    // We also don't load them if they are loading from cache because this will cost too much during launch
    if loadContentBlockers {
      let version = folderURL.lastPathComponent
      let blocklistType = ContentBlockerManager.BlocklistType.filterList(componentId: componentId, isAlwaysAggressive: isAlwaysAggressive)
      let modes = await blocklistType.allowedModes.asyncFilter { mode in
        if let loadedVersion = await FilterListStorage.shared.loadedRuleListVersions.value[componentId] {
          // if we know the loaded version we can just check it (optimization)
          return loadedVersion != version
        } else {
          return true
        }
      }
      
      // No modes need to be compiled
      guard !modes.isEmpty else { return }
      
      do {
        let filterSet = try String(contentsOf: filterListURL, encoding: .utf8)
        let result = try AdblockEngine.contentBlockerRules(fromFilterSet: filterSet)
        
        try await ContentBlockerManager.shared.compile(
          encodedContentRuleList: result.rulesJSON, for: blocklistType,
          options: .all, modes: modes
        )
        
        await MainActor.run {
          FilterListStorage.shared.loadedRuleListVersions.value[componentId] = version
        }
      } catch {
        ContentBlockerManager.log.error(
          "Failed to create content blockers for `\(componentId)` v\(version): \(error)"
        )
        #if DEBUG
        ContentBlockerManager.log.debug(
          "`\(componentId)`: \(filterListURL.absoluteString)"
        )
        #endif
      }
    }
  }
}

/// Helpful extension to the AdblockService
private extension AdblockService {
  /// Stream the URL updates to the `shieldsInstallPath`
  ///
  /// - Warning: You should never do this more than once. Only one callback can be registered to the `shieldsComponentReady` callback.
  @MainActor var shieldsInstallURL: AsyncStream<URL> {
    return AsyncStream { continuation in
      if let folderPath = shieldsInstallPath {
        let folderURL = URL(fileURLWithPath: folderPath)
        continuation.yield(folderURL)
      }
      
      guard shieldsComponentReady == nil else {
        assertionFailure("You have already set the `shieldsComponentReady` callback. Setting this more than once replaces the previous callback.")
        return
      }
      
      shieldsComponentReady = { folderPath in
        guard let folderPath = folderPath else {
          return
        }
        
        let folderURL = URL(fileURLWithPath: folderPath)
        continuation.yield(folderURL)
      }
      
      continuation.onTermination = { @Sendable _ in
        self.shieldsComponentReady = nil
      }
    }
  }
  
  /// Register the filter list given by the uuid and streams its updates
  ///
  /// - Note: Cancelling this task will unregister this filter list from recieving any further updates
  @MainActor func register(filterList: FilterList) -> AsyncStream<URL?> {
    return AsyncStream { continuation in
      registerFilterListComponent(filterList.entry, useLegacyComponent: false) { folderPath in
        guard let folderPath = folderPath else {
          continuation.yield(nil)
          return
        }
        
        let folderURL = URL(fileURLWithPath: folderPath)
        continuation.yield(folderURL)
      }
      
      continuation.onTermination = { @Sendable _ in
        self.unregisterFilterListComponent(filterList.entry, useLegacyComponent: true)
      }
    }
  }
}
