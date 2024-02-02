// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Combine
import Data
import Foundation
import Preferences
import Shared
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
    guard
      let resourcesFolderURL = await FilterListSetting.makeFolderURL(
        forComponentFolderPath: Preferences.AppState.lastAdBlockResourcesFolderPath.value
      ), FileManager.default.fileExists(atPath: resourcesFolderURL.path)
    else {
      // We need this for all filter lists so we can't compile anything until we download it
      return
    }

    let resourcesInfo = await didUpdateResourcesComponent(folderURL: resourcesFolderURL)
    async let cachedFilterLists: Void = compileCachedFilterLists(resourcesInfo: resourcesInfo)
    _ = await (cachedFilterLists)
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
        guard await setting.isEagerlyLoaded == true else { return }
        guard let componentId = await setting.componentId else { return }
        guard !FilterList.disabledComponentIDs.contains(componentId) else { return }
        guard let source = await setting.engineSource else { return }

        // Try to load the filter list folder. We always have to compile this at start
        guard let folderURL = await setting.folderURL,
          FileManager.default.fileExists(atPath: folderURL.path)
        else {
          return
        }

        await self.compileFilterListEngineIfNeeded(
          source: source,
          folderURL: folderURL,
          isAlwaysAggressive: setting.isAlwaysAggressive,
          resourcesInfo: resourcesInfo,
          compileContentBlockers: false
        )

        // Sleep for 1ms. This drastically reduces memory usage without much impact to usability
        try await Task.sleep(nanoseconds: 1_000_000)
      }
    } catch {
      // Ignore the cancellation.
    }
  }

  /// Start the adblock service to get updates to the `shieldsInstallPath`
  public func start(with adBlockService: AdblockService) {
    self.adBlockService = adBlockService

    // Here we register components in a specific order in order to avoid race conditions:
    // 1. All our filter lists need the resources component. So we register this first
    // 2. We also register the catalogue component since we don't need the resources for this
    // 3. When either of the above triggers we check if we have our resources and catalogue
    // and if we do have both we register all of the filter lists.
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

        ContentBlockerManager.log.debug("Loaded filter list catalog")
        if await AdBlockStats.shared.resourcesInfo != nil {
          await registerAllFilterListsIfNeeded(with: adBlockService)
        }
      }
    }
  }

  /// Register all enabled filter lists and to the default filter list with the `AdBlockService`
  private func registerAllFilterListsIfNeeded(with adBlockService: AdblockService) async {
    guard !registeredFilterLists else { return }
    self.registeredFilterLists = true

    for filterList in await FilterListStorage.shared.filterLists {
      register(filterList: filterList)
    }
  }

  @discardableResult
  /// When the
  private func didUpdateResourcesComponent(
    folderURL: URL
  ) async -> CachedAdBlockEngine.ResourcesInfo {
    await Task { @MainActor in
      let folderSubPath = FilterListSetting.extractFolderPath(fromComponentFolderURL: folderURL)
      Preferences.AppState.lastAdBlockResourcesFolderPath.value = folderSubPath
    }.value

    let version = folderURL.lastPathComponent
    let resourcesInfo = CachedAdBlockEngine.ResourcesInfo(
      localFileURL: folderURL.appendingPathComponent("resources.json", conformingTo: .json),
      version: version
    )

    await AdBlockStats.shared.updateIfNeeded(resourcesInfo: resourcesInfo)
    return resourcesInfo
  }

  /// Compile the engine from the given `AdblockService` `shieldsInstallPath` `URL`.
  private func compileEngine(
    filterListFolderURL folderURL: URL,
    resourcesInfo: CachedAdBlockEngine.ResourcesInfo,
    engineSource: CachedAdBlockEngine.Source,
    isAlwaysAggressive: Bool,
    compileContentBlockers: Bool
  ) async {
    let localFileURL = folderURL.appendingPathComponent("list.txt", conformingTo: .text)

    let version = folderURL.lastPathComponent
    let filterListInfo = CachedAdBlockEngine.FilterListInfo(
      source: engineSource,
      localFileURL: localFileURL,
      version: version,
      fileType: .text
    )
    let lazyInfo = AdBlockStats.LazyFilterListInfo(
      filterListInfo: filterListInfo,
      isAlwaysAggressive: isAlwaysAggressive
    )

    await AdBlockStats.shared.compile(
      lazyInfo: lazyInfo,
      resourcesInfo: resourcesInfo,
      compileContentBlockers: compileContentBlockers
    )
  }

  /// Load general filter lists (shields) from the given `AdblockService` `shieldsInstallPath` `URL`.
  private func compileFilterListEngineIfNeeded(
    source: CachedAdBlockEngine.Source,
    folderURL: URL,
    isAlwaysAggressive: Bool,
    resourcesInfo: CachedAdBlockEngine.ResourcesInfo,
    compileContentBlockers: Bool
  ) async {
    let version = folderURL.lastPathComponent
    let filterListURL = folderURL.appendingPathComponent("list.txt", conformingTo: .text)

    guard FileManager.default.fileExists(atPath: filterListURL.relativePath) else {
      // We are loading the old component from cache. We don't want this file to be loaded.
      // When we download the new component shortly we will update our cache.
      // This should only trigger after an app update and eventually this check can be removed.
      return
    }

    let filterListInfo = CachedAdBlockEngine.FilterListInfo(
      source: source,
      localFileURL: filterListURL,
      version: version,
      fileType: .text
    )
    let lazyInfo = AdBlockStats.LazyFilterListInfo(
      filterListInfo: filterListInfo,
      isAlwaysAggressive: isAlwaysAggressive
    )

    //  Check if we should load these rules
    guard await AdBlockStats.shared.isEnabled(source: source) else {
      await AdBlockStats.shared.updateIfNeeded(
        filterListInfo: filterListInfo,
        isAlwaysAggressive: isAlwaysAggressive
      )

      // To free some space, remove any rule lists that are not needed
      if let blocklistType = lazyInfo.blocklistType {
        do {
          try await ContentBlockerManager.shared.removeRuleLists(for: blocklistType)
        } catch {
          ContentBlockerManager.log.error(
            "Failed to remove rule lists for \(filterListInfo.debugDescription)"
          )
        }
      }

      return
    }

    await AdBlockStats.shared.compile(
      lazyInfo: lazyInfo,
      resourcesInfo: resourcesInfo,
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
        guard let resourcesInfo = await AdBlockStats.shared.resourcesInfo else {
          assertionFailure("We shouldn't have started downloads before getting this value")
          return
        }
        let source = filterList.engineSource

        // Add or remove the filter list from the engine depending if it's been enabled or not
        await self.compileFilterListEngineIfNeeded(
          source: source,
          folderURL: folderURL,
          isAlwaysAggressive: filterList.isAlwaysAggressive,
          resourcesInfo: resourcesInfo,
          compileContentBlockers: true
        )

        // Save the downloaded folder for later (caching) purposes
        FilterListStorage.shared.set(folderURL: folderURL, forUUID: filterList.entry.uuid)
      }
    }
  }
}

/// Helpful extension to the AdblockService
extension AdblockService {
  @MainActor fileprivate func resourcesComponentStream() -> AsyncStream<URL?> {
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

  @MainActor fileprivate func filterListCatalogComponentStream() -> AsyncStream<
    [AdblockFilterListCatalogEntry]
  > {
    return AsyncStream { continuation in
      registerFilterListCatalogComponent { filterListEntries in
        continuation.yield(filterListEntries)
      }
    }
  }

  /// Register the filter list given by the uuid and streams its updates
  ///
  /// - Note: Cancelling this task will unregister this filter list from recieving any further updates
  @MainActor fileprivate func register(filterList: FilterList) -> AsyncStream<URL?> {
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
