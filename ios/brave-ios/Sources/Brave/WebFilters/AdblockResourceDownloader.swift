// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import Shared
import os.log

/// A class responsible for downloading some generic ad-block resources
public actor AdblockResourceDownloader: Sendable {
  public static let shared = AdblockResourceDownloader()

  /// All the different resources this downloader handles
  static let handledResources: [BraveS3Resource] = [.adBlockRules]

  /// A list of old resources that need to be deleted so as not to take up the user's disk space
  private static let deprecatedResources: [BraveS3Resource] = [.deprecatedGeneralCosmeticFilters]

  /// The resource downloader that will be used to download all our resoruces
  private let resourceDownloader: ResourceDownloader<BraveS3Resource>

  init(networkManager: NetworkManager = NetworkManager()) {
    self.resourceDownloader = ResourceDownloader(networkManager: networkManager)
  }

  /// This will load cached and bundled data for the allowed modes.
  func loadCachedAndBundledDataIfNeeded(allowedModes: Set<ContentBlockerManager.BlockingMode>) async
  {
    guard !allowedModes.isEmpty else { return }
    await loadBundledDataIfNeeded(allowedModes: allowedModes)
  }

  /// This will load bundled data for the given content blocking modes. But only if the files are not already compiled.
  private func loadBundledDataIfNeeded(allowedModes: Set<ContentBlockerManager.BlockingMode>) async
  {
    // Compile bundled blocklists but only if we don't have anything already loaded.
    await ContentBlockerManager.GenericBlocklistType.allCases.asyncConcurrentForEach {
      genericType in
      let blocklistType = ContentBlockerManager.BlocklistType.generic(genericType)
      let modes = await ContentBlockerManager.shared.missingModes(
        for: blocklistType,
        version: genericType.version
      )

      do {
        try await ContentBlockerManager.shared.compileBundledRuleList(
          for: genericType,
          modes: modes
        )
      } catch {
        assertionFailure("A bundled file should not fail to compile")
      }
    }
  }

  /// Start fetching resources
  public func startFetching() {
    let fetchInterval = AppConstants.isOfficialBuild ? 6.hours : 10.minutes

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
      for try await result in await self.resourceDownloader.downloadStream(
        for: resource,
        every: fetchInterval
      ) {
        switch result {
        case .success(let downloadResult):
          await self.handle(
            downloadResult: downloadResult,
            for: resource,
            allowedModes: Set(ContentBlockerManager.BlockingMode.allCases)
          )
        case .failure(let error):
          ContentBlockerManager.log.error(
            "Failed to fetch resource `\(resource.cacheFileName)`: \(error.localizedDescription)"
          )
        }
      }
    }
  }

  /// Handle the downloaded file url for the given resource
  private func handle(
    downloadResult: ResourceDownloader<BraveS3Resource>.DownloadResult,
    for resource: BraveS3Resource,
    allowedModes: Set<ContentBlockerManager.BlockingMode>
  ) async {
    switch resource {
    case .adBlockRules:
      let blocklistType = ContentBlockerManager.BlocklistType.generic(.blockAds)
      var modes = blocklistType.allowedModes

      if !downloadResult.isModified && !allowedModes.isEmpty {
        // If the download is not modified, only compile the missing modes for performance reasons
        let missingModes = await ContentBlockerManager.shared.missingModes(
          for: blocklistType,
          version: downloadResult.version
        )
        modes = missingModes.filter({ allowedModes.contains($0) })
      }

      // No modes are needed to be compiled
      guard !modes.isEmpty else { return }

      guard let fileURL = resource.downloadedFileURL else {
        assertionFailure("This file was downloaded successfully so it should not be nil")
        return
      }

      // try to compile
      await ContentBlockerManager.shared.compileRuleList(
        at: fileURL,
        for: blocklistType,
        version: downloadResult.version,
        modes: modes
      )
    case .deprecatedGeneralCosmeticFilters:
      assertionFailure("Should not be handling this resource type")
    }
  }
}
