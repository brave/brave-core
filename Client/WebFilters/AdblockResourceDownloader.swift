// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Shared
import BraveShared

private let log = Logger.browserLogger

/// A class responsible for downloading some generic ad-block resources
public actor AdblockResourceDownloader: Sendable {
  public static let shared = AdblockResourceDownloader()
  
  /// A formatter that is used to format a version number
  private let fileVersionDateFormatter: DateFormatter = {
    let dateFormatter = DateFormatter()
    dateFormatter.locale = Locale(identifier: "en_US_POSIX")
    dateFormatter.dateFormat = "yyyy.MM.dd.HH.mm.ss"
    dateFormatter.timeZone = TimeZone(secondsFromGMT: 0)
    return dateFormatter
  }()
  
  /// The resource downloader that will be used to download all our resoruces
  private let resourceDownloader: ResourceDownloader
  /// All the resources that this downloader handles
  private let handledResources: [ResourceDownloader.Resource] = [.genericContentBlockingBehaviors, .generalCosmeticFilters]

  init(networkManager: NetworkManager = NetworkManager()) {
    self.resourceDownloader = ResourceDownloader(networkManager: networkManager)
  }
  
  /// Load the cached data and await the results
  public func loadCachedData() async {
    await withTaskGroup(of: Void.self) { group in
      for resource in handledResources {
        group.addTask {
          await self.loadCachedData(for: resource)
        }
      }
    }
  }

  /// Start fetching resources
  public func startFetching() {
    let fetchInterval = AppConstants.buildChannel.isPublic ? 6.hours : 10.minutes
    
    for resource in handledResources {
      startFetching(resource: resource, every: fetchInterval)
    }
  }
  
  /// Start fetching the given resource at regular intervals
  private func startFetching(resource: ResourceDownloader.Resource, every fetchInterval: TimeInterval) {
    Task { @MainActor in
      if let fileURL = ResourceDownloader.downloadedFileURL(for: resource) {
        let date = try ResourceDownloader.creationDate(for: resource)
        await self.handle(downloadedFileURL: fileURL, for: resource, date: date)
      }
      
      for try await result in await self.resourceDownloader.downloadStream(for: resource, every: fetchInterval) {
        switch result {
        case .success(let downloadResult):
          await self.handle(downloadedFileURL: downloadResult.fileURL, for: resource, date: downloadResult.date)
        case .failure(let error):
          log.error(error)
        }
      }
    }
  }
  
  /// Load cached data for the given resource. Ensures this is done on the MainActor
  private func loadCachedData(for resource: ResourceDownloader.Resource) async {
    if let fileURL = ResourceDownloader.downloadedFileURL(for: resource) {
      await handle(downloadedFileURL: fileURL, for: resource)
    }
  }
  
  /// Handle the downloaded file url for the given resource
  private func handle(downloadedFileURL: URL, for resource: ResourceDownloader.Resource, date: Date? = nil) async {
    let version = date != nil ? fileVersionDateFormatter.string(from: date!) : nil
    
    switch resource {
    case .genericFilterRules:
      await AdBlockEngineManager.shared.add(
        resource: AdBlockEngineManager.Resource(type: .ruleList, source: .adBlock),
        fileURL: downloadedFileURL,
        version: version
      )
      
    case .generalCosmeticFilters:
      await AdBlockEngineManager.shared.add(
        resource: AdBlockEngineManager.Resource(type: .dat, source: .cosmeticFilters),
        fileURL: downloadedFileURL,
        version: version
      )
      
    case .genericContentBlockingBehaviors:
      await ContentBlockerManager.shared.set(resource: ContentBlockerManager.Resource(
        url: downloadedFileURL,
        sourceType: .downloaded(version: version)
      ), for: .general(.blockAds))
      
    default:
      assertionFailure("Should not be handling this resource type")
    }
  }
}
