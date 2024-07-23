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
  static let handledResources: [BraveS3Resource] = {
    return [.slimList]
  }()

  /// A list of old resources that need to be deleted so as not to take up the user's disk space
  private static let deprecatedResources: [BraveS3Resource] = {
    // TODO: @JS Add the `adBlockRules` here once slim list is dropped
    return []
  }()

  /// The resource downloader that will be used to download all our resoruces
  private let resourceDownloader: ResourceDownloader<BraveS3Resource>

  init(networkManager: NetworkManager = NetworkManager()) {
    self.resourceDownloader = ResourceDownloader(networkManager: networkManager)
  }

  /// Start fetching resources
  public func startFetching() async {
    let fetchInterval = AppConstants.isOfficialBuild ? 6.hours : 10.minutes

    for resource in Self.handledResources {
      startFetching(resource: resource, every: fetchInterval)
    }

    // Remove any old files
    // We can remove this code in some not-so-distant future
    Task(priority: .low) {
      for resource in Self.deprecatedResources {
        do {
          try await resource.removeCacheFolder()
        } catch {
          ContentBlockerManager.log.error(
            "Failed to removed deprecated file \(resource.cacheFileName): \(error)"
          )
        }
      }
    }
  }

  /// Perform a one time force update of all the resources
  public func updateResources() async {
    await Self.handledResources.asyncConcurrentForEach { resource in
      do {
        let result = try await self.resourceDownloader.download(resource: resource, force: true)
        await self.handle(
          downloadResult: result,
          for: resource,
          allowedModes: Set(ContentBlockerManager.BlockingMode.allCases)
        )
      } catch {
        ContentBlockerManager.log.error(
          "Failed to fetch resource `\(resource.cacheFileName)`: \(error.localizedDescription)"
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
    case .slimList:
      guard let fileURL = resource.downloadedFileURL else {
        assertionFailure("This file was downloaded successfully so it should not be nil")
        return
      }

      await AdBlockGroupsManager.shared.update(
        fileInfo: AdBlockEngineManager.FileInfo(
          filterListInfo: GroupedAdBlockEngine.FilterListInfo(
            source: .slimList,
            version: downloadResult.version
          ),
          localFileURL: fileURL
        )
      )
      await AdBlockGroupsManager.shared.compileIfFilesAreReady(for: .standard)
    }
  }
}
