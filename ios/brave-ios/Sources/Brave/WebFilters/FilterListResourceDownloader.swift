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

        await AdBlockGroupsManager.shared.didUpdateResourcesComponent(folderURL: folderURL)
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

        if AdBlockGroupsManager.shared.resourcesInfo != nil {
          await registerAllFilterListsIfNeeded(with: adBlockService)
        }
      }
    }
  }

  /// Register all enabled filter lists and to the default filter list with the `AdBlockService`
  private func registerAllFilterListsIfNeeded(with adBlockService: AdblockService) async {
    guard !registeredFilterLists else { return }
    self.registeredFilterLists = true

    // First prioritize default filter list
    for filterList in await FilterListStorage.shared.filterLists
    where filterList.isEnabled && filterList.engineType == .standard {
      register(filterList: filterList)
    }

    // Next prioritize the additional lists
    try? await Task.sleep(seconds: 5)
    for filterList in await FilterListStorage.shared.filterLists
    where filterList.isEnabled && filterList.engineType == .aggressive {
      register(filterList: filterList)
    }

    // Next get the remaining filter lists after a short delay
    try? await Task.sleep(seconds: 5)
    for filterList in await FilterListStorage.shared.filterLists where !filterList.isEnabled {
      register(filterList: filterList)
    }
  }

  /// Load general filter lists (shields) from the given `AdblockService` `shieldsInstallPath` `URL`.
  private func compileFilterListEngineIfNeeded(
    source: GroupedAdBlockEngine.Source,
    folderURL: URL,
    engineType: GroupedAdBlockEngine.EngineType
  ) async {
    guard
      let fileInfo = AdBlockEngineManager.FileInfo(
        for: source,
        downloadedFolderURL: folderURL
      )
    else {
      return
    }

    ContentBlockerManager.log.debug(
      "Updated filter list: \(fileInfo.filterListInfo.debugDescription)"
    )

    await AdBlockGroupsManager.shared.update(
      fileInfo: fileInfo,
      engineType: engineType,
      compileDelayed: true
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
        let source = filterList.engineSource

        // Add or remove the filter list from the engine depending if it's been enabled or not
        await self.compileFilterListEngineIfNeeded(
          source: source,
          folderURL: folderURL,
          engineType: filterList.engineType
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
