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
@MainActor public class FilterListResourceDownloader {
  /// A shared instance of this class
  ///
  /// - Warning: You need to wait for `DataController.shared.initializeOnce()` to be called before using this instance
  public static let shared = FilterListResourceDownloader()
  /// A marker that says that we loaded shield components for the first time.
  /// This boolean is used to configure this downloader only once after `AdBlockService` generic shields have been loaded.
  private var subscribedToFilterListChange = false
  /// The filter list change subscription
  private var filterListSubscription: AnyCancellable?
  /// This is a task that we use to delay a requested compile.
  /// This allows us to wait for more sources and limit the number of times we compile
  private var delayTasks: [GroupedAdBlockEngine.EngineType: Task<Void, Error>] = [:]

  init() {}

  /// Start the adblock service to get adblock file updates
  public func start(with adBlockService: AdblockService) {
    if let resourcesPath = adBlockService.resourcesPath {
      // Do an initial load of resources
      AdBlockGroupsManager.shared.didUpdateResourcesComponent(
        resourcesFileURL: resourcesPath
      )
    }

    subscribeToResourceChanges(with: adBlockService)
    subscribeToFilterListChanges(with: adBlockService)
  }

  private func subscribeToResourceChanges(with adBlockService: AdblockService) {
    Task {
      for await resourcesFileURL in adBlockService.resourcesComponentStream() {
        AdBlockGroupsManager.shared.didUpdateResourcesComponent(
          resourcesFileURL: resourcesFileURL
        )
      }
    }
  }

  /// Subscribe to filter list changes so we keep the engines up to date
  private func subscribeToFilterListChanges(with adBlockService: AdblockService) {
    guard !subscribedToFilterListChange else { return }
    self.subscribedToFilterListChange = true
    filterListSubscription = FilterListStorage.shared.$filterLists
      .receive(on: DispatchQueue.main)
      .sink { [weak self] _ in
        Task { @MainActor in
          for engineType in GroupedAdBlockEngine.EngineType.allCases {
            self?.updateEnginesDelayed(for: engineType, adBlockService: adBlockService)
          }
        }
      }

    Task {
      for await engineType in adBlockService.filterListChangesStream() {
        ContentBlockerManager.log.debug(
          "Downloaded filter lists for `\(engineType.debugDescription)`"
        )

        // In order to improve the speed our youtube ad-blocking is ready,
        // we prioritize the `standard` engine and don't delay it
        // unless we already have the engine
        if engineType == .standard && !AdBlockGroupsManager.shared.hasEngine(for: engineType) {
          updateEngines(for: engineType, adBlockService: adBlockService)
        } else {
          updateEnginesDelayed(for: engineType, adBlockService: adBlockService)
        }
      }
    }
  }

  /// Updated the engine managers with file infos available for the given engine type.
  /// This will delay the update by several seconds in order to prevent excessive compilation of engines
  ///
  /// This prevents too many compilations of the engine if the user is changing settings
  /// while still updating the engine in a timely manner
  private func updateEnginesDelayed(
    for engineType: GroupedAdBlockEngine.EngineType,
    adBlockService: AdblockService
  ) {
    delayTasks[engineType]?.cancel()
    delayTasks[engineType] = Task {
      // A short delay because this gets triggered way to frequently
      try await Task.sleep(seconds: 5)
      delayTasks.removeValue(forKey: engineType)
      let fileInfos = adBlockService.fileInfos(for: engineType)
      AdBlockGroupsManager.shared.update(fileInfos: fileInfos)
      AdBlockGroupsManager.shared.compileIfFilesAreReady(for: engineType)
    }
  }

  /// Updated the engine managers with file infos available for the given engine type
  ///
  /// Note: You should call this infrequently as it may cause lots of compilations.
  /// Instead use `updateEnginesDelayed` unless you absolutely need an engine ready right away
  /// such as on a first launch
  private func updateEngines(
    for engineType: GroupedAdBlockEngine.EngineType,
    adBlockService: AdblockService
  ) {
    let fileInfos = adBlockService.fileInfos(for: engineType)
    AdBlockGroupsManager.shared.update(fileInfos: fileInfos)
    AdBlockGroupsManager.shared.compileIfFilesAreReady(for: engineType)
  }
}

/// Helpful extension to the AdblockService
extension AdblockService {
  /// Get a stream of resource component updates
  @MainActor fileprivate func resourcesComponentStream() -> AsyncStream<URL> {
    return AsyncStream { continuation in
      registerResourcesChanges { [weak self] _ in
        guard let resourcesPath = self?.resourcesPath else {
          return
        }

        continuation.yield(resourcesPath)
      }
    }
  }

  /// Get a stream that informs us of filter list file updates
  @MainActor fileprivate func filterListChangesStream() -> AsyncStream<
    GroupedAdBlockEngine.EngineType
  > {
    return AsyncStream { continuation in
      registerFilterListChanges({ isDefaultFilterList in
        continuation.yield(isDefaultFilterList ? .standard : .aggressive)
      })
    }
  }
}

extension AdblockFilterListCatalogEntry {
  /// Create file info for this entry
  func fileInfo(for localFileURL: URL) -> AdBlockEngineManager.FileInfo {
    let version = localFileURL.deletingLastPathComponent().lastPathComponent

    return AdBlockEngineManager.FileInfo(
      filterListInfo: GroupedAdBlockEngine.FilterListInfo(
        source: engineSource,
        version: version
      ),
      localFileURL: localFileURL
    )
  }
}
