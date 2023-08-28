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
public class FilterListResourceDownloader {
  /// A shared instance of this class
  ///
  /// - Warning: You need to wait for `DataController.shared.initializeOnce()` to be called before using this instance
  public static let shared = FilterListResourceDownloader()
  
  /// Object responsible for getting component updates
  private var adBlockService: AdblockService?
  /// The filter list subscription
  private var filterListSubscription: AnyCancellable?
  /// Ad block service tasks per filter list UUID
  private var adBlockServiceTasks: [String: Task<Void, Error>]
  /// A marker that says that we loaded shield components for the first time.
  /// This boolean is used to configure this downloader only once after `AdBlockService` generic shields have been loaded.
  private var loadedShieldComponents = false
  
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
    async let cachedFilterLists: Void = self.addEngineResourcesFromCachedFilterLists()
    async let cachedGeneralFilterList: Void = self.addEngineResourcesFromCachedGeneralFilterList()
    _ = await (cachedFilterLists, cachedGeneralFilterList)
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
  private func addEngineResourcesFromCachedFilterLists() async {
    await FilterListStorage.shared.loadFilterListSettings()
    let filterListSettings = await FilterListStorage.shared.allFilterListSettings
      
    await filterListSettings.asyncConcurrentForEach { setting in
      guard await setting.isEnabled == true else { return }
      guard let componentId = await setting.componentId else { return }
      
      // Try to load the filter list folder. We always have to compile this at start
      guard let folderURL = await setting.folderURL, FileManager.default.fileExists(atPath: folderURL.path) else {
        return
      }
      
      // Because this is called during launch, we don't want to recompile content blockers.
      // This is because recompiling content blockers is too expensive and they are lazily loaded anyways.
      await self.loadShields(
        fromComponentId: componentId, folderURL: folderURL,
        relativeOrder: setting.order?.intValue ?? 0,
        loadContentBlockers: false,
        isAlwaysAggressive: setting.isAlwaysAggressive
      )
    }
  }
  
  /// This function adds engine resources to `AdBlockManager` from the "general" filter list.
  ///
  /// The "general" filter list is a built in filter list that is added to the`AdBlockEngine` and as content blockers.
  /// It represents the "Block cross site trackers" toggle in the "Shields" menu.
  ///
  /// - Note: The content blockers for this "general" filter list are handled using the `AdBlockResourceDownloader` and are not loaded here at this point.
  private func addEngineResourcesFromCachedGeneralFilterList() async {
    guard let folderURL = await FilterListSetting.makeFolderURL(
      forFilterListFolderPath: Preferences.AppState.lastDefaultFilterListFolderPath.value
    ), FileManager.default.fileExists(atPath: folderURL.path) else {
      return
    }
    
    await addEngineResources(fromGeneralFilterListFolderURL: folderURL)
  }
  
  /// Start the adblock service to get updates to the `shieldsInstallPath`
  @MainActor public func start(with adBlockService: AdblockService) {
    self.adBlockService = adBlockService
    
    // Start listening to changes to the install url
    Task {
      for await folderURL in adBlockService.shieldsInstallURL {
        self.didUpdateShieldComponent(
          folderURL: folderURL,
          adBlockFilterLists: adBlockService.regionalFilterLists ?? []
        )
      }
    }
  }
  
  /// Invoked when shield components are loaded
  ///
  /// This function will start fetching data and subscribe publishers once if it hasn't already done so.
  @MainActor private func didUpdateShieldComponent(folderURL: URL, adBlockFilterLists: [AdblockFilterListCatalogEntry]) {
    if !loadedShieldComponents && !adBlockFilterLists.isEmpty {
      // This is the first time we load ad-block filters.
      // We need to perform some initial setup (but only do this once)
      loadedShieldComponents = true
      FilterListStorage.shared.loadFilterLists(from: adBlockFilterLists)
      
      self.subscribeToFilterListChanges()
      self.registerAllEnabledFilterLists()
    }
    
    // Store the folder path so we can load it from cache next time we launch quicker
    // than waiting for the component updater to respond, which may take a few seconds
    let folderSubPath = FilterListSetting.extractFolderPath(fromFilterListFolderURL: folderURL)
    Preferences.AppState.lastDefaultFilterListFolderPath.value = folderSubPath
    
    Task {
      await self.addEngineResources(fromGeneralFilterListFolderURL: folderURL)
    }
  }
  
  /// Load general filter lists (shields) from the given `AdblockService` `shieldsInstallPath` `URL`.
  private func addEngineResources(fromGeneralFilterListFolderURL folderURL: URL) async {
    let version = folderURL.lastPathComponent
    await AdBlockEngineManager.shared.set(scripletResourcesURL: folderURL.appendingPathComponent("resources.json"))
    
    // Lets add these new resources
    await AdBlockEngineManager.shared.add(
      resource: AdBlockEngineManager.Resource(type: .dat, source: .adBlock),
      fileURL: folderURL.appendingPathComponent("rs-ABPFilterParserData.dat"),
      version: version
    )
  }
  
  /// Subscribe to the changes on the `filterLists` on `FilterListStorage` so that we can save settings and register or unregister the filter lists as we change them in UI
  @MainActor private func subscribeToFilterListChanges() {
    // Subscribe to changes on the filter list states
    filterListSubscription = FilterListStorage.shared.$filterLists
      .receive(on: DispatchQueue.main)
      .sink { filterLists in
        for filterList in filterLists {
          self.handleUpdate(to: filterList)
        }
      }
  }
  
  /// Ensures settings are in sync with our `FilterListStorage` and we register to/unregister from the `AdBlockService`
  @MainActor private func handleUpdate(to filterList: FilterList) {
    // Register or unregister the filter list depending on its toggle state
    if filterList.isEnabled {
      register(filterList: filterList)
    } else {
      unregister(filterList: filterList)
    }
  }
  
  /// Register all enabled filter lists with the `AdBlockService`
  @MainActor private func registerAllEnabledFilterLists() {
    for filterList in FilterListStorage.shared.filterLists {
      guard filterList.isEnabled else { continue }
      register(filterList: filterList)
    }
  }
  
  /// Register this filter list with the `AdBlockService`
  @MainActor private func register(filterList: FilterList) {
    guard adBlockServiceTasks[filterList.entry.componentId] == nil else { return }
    guard let adBlockService = adBlockService else { return }

    adBlockServiceTasks[filterList.entry.componentId] = Task { @MainActor in
      for await folderURL in adBlockService.register(filterList: filterList) {
        guard let folderURL = folderURL else { continue }
        
        await self.loadShields(
          fromComponentId: filterList.entry.componentId, folderURL: folderURL, relativeOrder: filterList.order,
          loadContentBlockers: true,
          isAlwaysAggressive: filterList.isAlwaysAggressive
        )
        
        // Save the downloaded folder for later (caching) purposes
        FilterListStorage.shared.set(folderURL: folderURL, forUUID: filterList.uuid)
      }
    }
  }
  
  /// Unregister this filter list by cancelling its registration task on `AdBlockService` and removing any `AdBlockEngineManager` resources for this filter list
  ///
  /// - Note: The corresponding rule is not removed from the `WKContentRuleListStore`.
  /// This is because removing the rules does not then allow us to remove them from the tab. (A bug in iOS perhaps?)
  /// Therefore we will remove the rules as a cleanup upon the next launch.
  /// The `ContentBlockerHelper` and `ContentBlockerManager` will take care of removing
  /// any disabled/deleted rules from the tab on the next page load so it doesn't really matter much if we don't delete it now.
  @MainActor private func unregister(filterList: FilterList) {
    adBlockServiceTasks[filterList.entry.componentId]?.cancel()
    adBlockServiceTasks.removeValue(forKey: filterList.entry.componentId)
    
    Task {
      await AdBlockEngineManager.shared.removeResources(
        for: .filterList(componentId: filterList.entry.componentId,
        isAlwaysAggressive: filterList.isAlwaysAggressive
      ))
    }
  }
  
  /// Handle the downloaded component folder url of a filter list.
  ///
  /// The folder URL should point to a `AdblockFilterListEntry` download location as given by the `AdBlockService`.
  ///
  /// If `loadContentBlockers` is set to `true`, this method will compile the rule lists to content blocker format and load them into the `WKContentRuleListStore`.
  /// As both these procedures are expensive, this should be set to `false` if this method is called on a blocking UI process such as the launch of the application.
  private func loadShields(fromComponentId componentId: String, folderURL: URL, relativeOrder: Int, loadContentBlockers: Bool, isAlwaysAggressive: Bool) async {
    // Check if we're loading the new component or an old component from cache.
    // The new component has a file `list.txt` which we check the presence of.
    let filterListURL = folderURL.appendingPathComponent("list.txt", conformingTo: .text)
    guard FileManager.default.fileExists(atPath: filterListURL.relativePath) else {
      // We are loading the old component from cache. We don't want this file to be loaded.
      // When we download the new component shortly we will update our cache.
      // This should only trigger after an app update and eventually this check can be removed.
      return
    }
    
    let version = folderURL.lastPathComponent
    
    // Add or remove the filter list from the engine depending if it's been enabled or not
    if await FilterListStorage.shared.isEnabled(for: componentId) {
      await AdBlockEngineManager.shared.add(
        resource: AdBlockEngineManager.Resource(
          type: .ruleList,
          source: .filterList(componentId: componentId, isAlwaysAggressive: isAlwaysAggressive)
        ),
        fileURL: filterListURL,
        version: version,
        relativeOrder: relativeOrder
      )
    } else {
      await AdBlockEngineManager.shared.removeResources(
        for: .filterList(componentId: componentId, isAlwaysAggressive: isAlwaysAggressive)
      )
    }
    
    // Compile this rule list if we haven't already or if the file has been modified
    // We also don't load them if they are loading from cache because this will cost too much during launch
    if loadContentBlockers {
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
          "Failed to convert filter list `\(componentId)` to content blockers: \(error)"
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
