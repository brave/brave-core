// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

/// This class helps to prepare the browser during launch by ensuring the state of managers, resources and downloaders before performing additional tasks.
public actor LaunchHelper {
  public static let shared = LaunchHelper()
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
      // Load cached data
      // This is done first because compileResources and loadCachedRuleLists need their results
      async let loadBundledResources: Void = ContentBlockerManager.shared.loadBundledResources()
      async let filterListCache: Void =  FilterListResourceDownloader.shared.loadCachedData()
      async let adblockResourceCache: Void = AdblockResourceDownloader.shared.loadCachedData()
      _ = await (loadBundledResources, filterListCache, adblockResourceCache)

      // Compile some engines and load cached rule lists
      async let compiledResourcesCompile: Void = AdBlockEngineManager.shared.compileResources()
      async let cachedRuleListLoad: Void = ContentBlockerManager.shared.loadCachedRuleLists()
      _ = await (compiledResourcesCompile, cachedRuleListLoad)
      
      // This one is non-blocking
      performPostLoadTasks(adBlockService: adBlockService)
      markAdBlockReady()
    }
    
    // Await the task and wait for the results
    self.loadTask = task
    await task.value
    self.loadTask = nil
  }
  
  private func markAdBlockReady() {
    areAdBlockServicesReady = true
  }
  
  private func performPostLoadTasks(adBlockService: AdblockService) {
    Task { @MainActor in
      await ContentBlockerManager.shared.cleanupDeadRuleLists()
      await ContentBlockerManager.shared.compilePendingResources()
      FilterListResourceDownloader.shared.start(with: adBlockService)
      await AdblockResourceDownloader.shared.startFetching()
      ContentBlockerManager.shared.startTimer()
      await AdBlockEngineManager.shared.startTimer()
    }
  }
}
