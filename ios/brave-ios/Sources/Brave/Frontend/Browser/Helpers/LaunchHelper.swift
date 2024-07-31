// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Foundation
import Preferences
import os

/// This class helps to prepare the browser during launch by ensuring the state of managers, resources and downloaders before performing additional tasks.
public actor LaunchHelper {
  public static let shared = LaunchHelper()
  static let signpost = OSSignposter(logger: ContentBlockerManager.log)
  private var loadTask: Task<(), Never>?
  private var areAdBlockServicesReady = false

  /// This method prepares the ad-block services one time so that multiple scenes can benefit from its results
  /// This is particularly important since we use a shared instance for most of our ad-block services.
  public func prepareAdBlockServices(adBlockService: AdblockService) async {
    // Check if ad-block services are already ready.
    // If so, we don't have to do anything
    guard !areAdBlockServicesReady else { return }

    // Check if we're still preparing the ad-block services
    // If so we await that task
    if let task = loadTask {
      return await task.value
    }

    // Otherwise prepare the services and await the task
    let task = Task {
      let signpostID = Self.signpost.makeSignpostID()
      ContentBlockerManager.log.debug("Loading blocking launch data")
      let state = Self.signpost.beginInterval("blockingLaunchTask", id: signpostID)
      await FilterListStorage.shared.start(with: adBlockService)

      // Load cached data
      // This is done first because compileResources need their results
      await AdBlockGroupsManager.shared.loadResourcesFromCache()
      async let loadEngines: Void = AdBlockGroupsManager.shared.loadEnginesFromCache()
      async let adblockResourceCache: Void = AdBlockGroupsManager.shared.loadBundledDataIfNeeded()
      _ = await (loadEngines, adblockResourceCache)
      Self.signpost.emitEvent("loadedCachedData", id: signpostID, "Loaded cached data")

      ContentBlockerManager.log.debug("Loaded blocking launch data")

      // This one is non-blocking
      performPostLoadTasks(adBlockService: adBlockService)
      areAdBlockServicesReady = true
      Self.signpost.endInterval("blockingLaunchTask", state)
    }

    // Await the task and wait for the results
    self.loadTask = task
    await task.value
    self.loadTask = nil
  }

  /// Perform tasks that don't need to block the initial load (things that can happen happily in the background after the first page loads
  private func performPostLoadTasks(
    adBlockService: AdblockService
  ) {
    Task.detached(priority: .low) {
      let signpostID = Self.signpost.makeSignpostID()
      let state = Self.signpost.beginInterval("nonBlockingLaunchTask", id: signpostID)
      await FilterListResourceDownloader.shared.start(with: adBlockService)
      await FilterListCustomURLDownloader.shared.startFetching()
      await AdblockResourceDownloader.shared.startFetching()
      // It's important to do this at the end to ensure we have our lists loaded
      await AdBlockGroupsManager.shared.cleaupInvalidRuleLists()
      Self.signpost.endInterval("nonBlockingLaunchTask", state)
    }
  }
}

extension FilterListStorage {
  /// Return all the blocklist types that are valid for filter lists.
  fileprivate var validBlocklistTypes: Set<ContentBlockerManager.BlocklistType> {
    if filterLists.isEmpty {
      // If we don't have filter lists yet loaded, use the settings
      return Set(
        allFilterListSettings.compactMap { setting -> ContentBlockerManager.BlocklistType? in
          return setting.engineSource?.blocklistType(
            engineType: setting.engineType
          )
        }
      )
    } else {
      // If we do have filter lists yet loaded, use them as they are always the most up to date and accurate
      return Set(
        filterLists.compactMap { filterList in
          return filterList.engineSource.blocklistType(
            engineType: filterList.engineType
          )
        }
      )
    }
  }
}
extension ShieldLevel {
  /// Return a list of first launch content blocker modes that MUST be precompiled during launch
  fileprivate var firstLaunchBlockingModes: Set<ContentBlockerManager.BlockingMode> {
    switch self {
    case .standard, .disabled:
      // Disabled setting may be overriden per domain so we need to treat it as standard
      // Aggressive needs to be included because some filter lists are aggressive only
      return [.general, .standard, .aggressive]
    case .aggressive:
      // If we have aggressive mode enabled, we never use standard
      // (until we allow domain specific aggressive mode)
      return [.general, .aggressive]
    }
  }
}
