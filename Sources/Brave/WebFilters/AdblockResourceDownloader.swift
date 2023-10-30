// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Shared
import BraveShared
import os.log

/// A class responsible for downloading some generic ad-block resources
public actor AdblockResourceDownloader: Sendable {
  public static let shared = AdblockResourceDownloader()
  
  /// All the different resources this downloader handles
  static let handledResources: [BraveS3Resource] = [
    .adBlockRules, .debounceRules
  ]
  
  /// A list of old resources that need to be deleted so as not to take up the user's disk space
  private static let deprecatedResources: [BraveS3Resource] = [.deprecatedGeneralCosmeticFilters]
  
  /// The resource downloader that will be used to download all our resoruces
  private let resourceDownloader: ResourceDownloader<BraveS3Resource>

  init(networkManager: NetworkManager = NetworkManager()) {
    self.resourceDownloader = ResourceDownloader(networkManager: networkManager)
  }
  
  /// This will load cached and bundled data for the allowed modes.
  func loadCachedAndBundledDataIfNeeded(allowedModes: Set<ContentBlockerManager.BlockingMode>) async {
    guard !allowedModes.isEmpty else { return }
    await loadCachedDataIfNeeded(allowedModes: allowedModes)
    await loadBundledDataIfNeeded(allowedModes: allowedModes)
  }
  
  /// This will load bundled data for the given content blocking modes. But only if the files are not already compiled.
  private func loadBundledDataIfNeeded(allowedModes: Set<ContentBlockerManager.BlockingMode>) async {
    // Compile bundled blocklists but only if we don't have anything already loaded.
    await ContentBlockerManager.GenericBlocklistType.allCases.asyncConcurrentForEach { genericType in
      let blocklistType = ContentBlockerManager.BlocklistType.generic(genericType)
      let modes = await blocklistType.allowedModes.asyncFilter { mode in
        guard allowedModes.contains(mode) else { return false }
        // Non .blockAds can be recompiled safely because they are never replaced by downloaded files
        if genericType != .blockAds { return true }
        
        // .blockAds is special because it can be replaced by a downloaded file.
        // Hence we need to first check if it already exists.
        if await ContentBlockerManager.shared.hasRuleList(for: blocklistType, mode: mode) {
          return false
        } else {
          return true
        }
      }
      
      do {
        try await ContentBlockerManager.shared.compileBundledRuleList(for: genericType, modes: modes)
      } catch {
        assertionFailure("A bundled file should not fail to compile")
      }
    }
  }
  
  /// Load the cached data and await the results
  private func loadCachedDataIfNeeded(allowedModes: Set<ContentBlockerManager.BlockingMode>) async {
    // Here we load downloaded resources if we need to
    await Self.handledResources.asyncConcurrentForEach { resource in
      do {
        // Check if we have cached results for the given resource
        if let cachedResult = try resource.cachedResult() {
          await self.handle(downloadResult: cachedResult, for: resource, allowedModes: allowedModes)
        }
      } catch {
        ContentBlockerManager.log.error(
          "Failed to load cached data for resource \(resource.cacheFileName): \(error)"
        )
      }
    }
  }

  /// Start fetching resources
  public func startFetching() {
    let fetchInterval = AppConstants.buildChannel.isPublic ? 6.hours : 10.minutes
    
    for resource in Self.handledResources {
      startFetching(resource: resource, every: fetchInterval)
    }
    
    // Remove any old files
    // We can remove this code in some not-so-distant future
    for resource in Self.deprecatedResources {
      do {
        try resource.removeCacheFolder()
      } catch {
        ContentBlockerManager.log.error(
          "Failed to removed deprecated file \(resource.cacheFileName): \(error)"
        )
      }
    }
  }
  
  /// Start fetching the given resource at regular intervals
  private func startFetching(resource: BraveS3Resource, every fetchInterval: TimeInterval) {
    Task { @MainActor in
      for try await result in await self.resourceDownloader.downloadStream(for: resource, every: fetchInterval) {
        switch result {
        case .success(let downloadResult):
          await self.handle(
            downloadResult: downloadResult, for: resource,
            allowedModes: Set(ContentBlockerManager.BlockingMode.allCases)
          )
        case .failure(let error):
          ContentBlockerManager.log.error("Failed to fetch resource `\(resource.cacheFileName)`: \(error.localizedDescription)")
        }
      }
    }
  }
  
  /// Handle the downloaded file url for the given resource
  private func handle(downloadResult: ResourceDownloader<BraveS3Resource>.DownloadResult, for resource: BraveS3Resource, allowedModes: Set<ContentBlockerManager.BlockingMode>) async {
    switch resource {
    case .adBlockRules:
      let blocklistType = ContentBlockerManager.BlocklistType.generic(.blockAds)
      var modes = blocklistType.allowedModes
      
      if !downloadResult.isModified && !allowedModes.isEmpty {
        // If the download is not modified, only compile the missing modes for performance reasons
        let missingModes = await ContentBlockerManager.shared.missingModes(for: blocklistType)
        modes = missingModes.filter({ allowedModes.contains($0) })
      }

      // No modes are needed to be compiled
      guard !modes.isEmpty else { return }
      
      do {
        guard let fileURL = resource.downloadedFileURL else {
          assertionFailure("This file was downloaded successfully so it should not be nil")
          return
        }
        
        // try to compile
        try await ContentBlockerManager.shared.compileRuleList(
          at: fileURL, for: blocklistType, modes: modes
        )
      } catch {
        ContentBlockerManager.log.error(
          "Failed to compile rule lists for `\(blocklistType.debugDescription)`: \(error.localizedDescription)"
        )
      }
      
    case .debounceRules:
      // We don't want to setup the debounce rules more than once for the same cached file
      guard downloadResult.isModified || DebouncingService.shared.matcher == nil else {
        return
      }
      
      do {
        guard let data = try resource.downloadedData() else {
          assertionFailure("We just downloaded this file, how can it not be there?")
          return
        }
        
        try DebouncingService.shared.setup(withRulesJSON: data)
      } catch {
        ContentBlockerManager.log.error("Failed to setup debounce rules: \(error.localizedDescription)")
      }
      
    case .deprecatedGeneralCosmeticFilters:
      assertionFailure("Should not be handling this resource type")
    }
  }
}
