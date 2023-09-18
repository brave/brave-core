// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import Combine
import BraveCore
import BraveShields

/// An actor that handles the downloading of custom filter lists which are sourced via a URL
actor FilterListCustomURLDownloader: ObservableObject {
  /// An object representing a downloadable custom filter list.
  struct DownloadResource: Hashable, DownloadResourceInterface {
    let uuid: String
    let externalURL: URL
    
    var cacheFolderName: String {
      return ["custom-filter-lists", uuid].joined(separator: "/")
    }
    
    var cacheFileName: String {
      return [uuid, "json"].joined(separator: ".")
    }
    
    var headers: [String: String] {
      return ["Accept": "text/plain"]
    }
  }
  
  /// A formatter that is used to format a version number
  private let fileVersionDateFormatter: DateFormatter = {
    let dateFormatter = DateFormatter()
    dateFormatter.locale = Locale(identifier: "en_US_POSIX")
    dateFormatter.dateFormat = "yyyy.MM.dd.HH.mm.ss"
    dateFormatter.timeZone = TimeZone(secondsFromGMT: 0)
    return dateFormatter
  }()
  
  static let shared = FilterListCustomURLDownloader()
  
  /// The resource downloader that downloads our resources
  private let resourceDownloader: ResourceDownloader<DownloadResource>
  /// Fetch content blocking tasks per filter list
  private var fetchTasks: [DownloadResource: Task<Void, Error>]
  /// Tells us if we started this service
  private var startedService = false
  
  init(networkManager: NetworkManager = NetworkManager()) {
    self.resourceDownloader = ResourceDownloader(networkManager: networkManager)
    self.fetchTasks = [:]
  }
  
  /// Load any custom filter lists from cache so they are ready to use and start fetching updates.
  func start() async {
    guard !startedService else { return }
    self.startedService = true
    await CustomFilterListStorage.shared.loadCachedFilterLists()
    
    await CustomFilterListStorage.shared.filterListsURLs.asyncConcurrentForEach { customURL in
      let resource = await customURL.setting.resource
      
      do {
        if let cachedResult = try resource.cachedResult() {
          await self.handle(downloadResult: cachedResult, for: customURL)
        }
      } catch {
        let uuid = await customURL.setting.uuid
        ContentBlockerManager.log.error(
          "Failed to cached data for custom filter list `\(uuid)`: \(error)"
        )
      }
      
      // Always fetch this resource so it's ready if the user enables it.
      await self.startFetching(filterListCustomURL: customURL)
    }
  }
  
  /// Handle the download results of a custom filter list. This will process the download by compiling iOS rule lists and adding the rule list to the `AdblockEngineManager`.
  private func handle(downloadResult: ResourceDownloader<DownloadResource>.DownloadResult, for filterListCustomURL: FilterListCustomURL) async {
    let uuid = await filterListCustomURL.setting.uuid
    
    // Compile this rule list if we haven't already or if the file has been modified
    if downloadResult.isModified {
      do {
        let filterSet = try String(contentsOf: downloadResult.fileURL, encoding: .utf8)
        let result = try AdblockEngine.contentBlockerRules(fromFilterSet: filterSet)
        let type = ContentBlockerManager.BlocklistType.customFilterList(uuid: uuid)
        
        try await ContentBlockerManager.shared.compile(
          encodedContentRuleList: result.rulesJSON,
          for: type,
          options: .all
        )
        
        ContentBlockerManager.log.debug(
          "Loaded custom filter list content blockers: \(String(describing: type))"
        )
      } catch {
        ContentBlockerManager.log.error(
          "Failed to convert custom filter list to content blockers: \(error.localizedDescription)"
        )
      }
    }
      
    // Add/remove the resource depending on if it is enabled/disabled
    if await filterListCustomURL.setting.isEnabled {
      guard let resourcesInfo = await FilterListResourceDownloader.shared.resourcesInfo else {
        assertionFailure("This should not have been called if the resources are not ready")
        return
      }
      
      let version = fileVersionDateFormatter.string(from: downloadResult.date)
      let filterListInfo = CachedAdBlockEngine.FilterListInfo(
        source: .filterListURL(uuid: uuid),
        localFileURL: downloadResult.fileURL,
        version: version, fileType: .text
      )
      
      do {
        let engine = try await CachedAdBlockEngine.compile(
          filterListInfo: filterListInfo, resourcesInfo: resourcesInfo,
          isAlwaysAggressive: true
        )
        
        await AdBlockStats.shared.add(engine: engine)
        ContentBlockerManager.log.debug("Compiled engine for custom filter list `\(uuid)` v\(version)")
      } catch {
        ContentBlockerManager.log.error("Failed to compile engine for custom filter list `\(uuid)` v\(version): \(String(describing: error))")
      }
    } else {
      await AdBlockStats.shared.removeEngine(for: .filterListURL(uuid: uuid))
    }
  }
  
  /// Start fetching the resource for the given filter list. Once a new version is downloaded, the file will be processed using the `handle` method
  func startFetching(filterListCustomURL: FilterListCustomURL) async {
    guard startedService else { return }
    let resource = await filterListCustomURL.setting.resource
    
    guard fetchTasks[resource] == nil else {
      // We're already fetching for this filter list
      return
    }
    
    fetchTasks[resource] = Task {
      do {
        for try await result in await self.resourceDownloader.downloadStream(for: resource) {
          switch result {
          case .success(let downloadResult):
            // Update the data for UI purposes
            await CustomFilterListStorage.shared.update(
              filterListId: filterListCustomURL.id,
              with: .success(downloadResult.date)
            )
            
            // Handle the successful result so we parse the content blockers
            await self.handle(
              downloadResult: downloadResult,
              for: filterListCustomURL
            )
          case .failure(let error):
            // We don't want to keep refetching these types of failures
            let hardFailureCodes = [URLError.Code.badURL, .appTransportSecurityRequiresSecureConnection, .fileIsDirectory, .unsupportedURL]
            if let urlError = error as? URLError, hardFailureCodes.contains(urlError.code) {
              throw urlError
            }
            
            await CustomFilterListStorage.shared.update(
              filterListId: filterListCustomURL.id,
              with: .failure(error)
            )
          }
        }
      } catch is CancellationError {
        self.fetchTasks.removeValue(forKey: resource)
      } catch {
        self.fetchTasks.removeValue(forKey: resource)
        
        await CustomFilterListStorage.shared.update(
          filterListId: filterListCustomURL.id,
          with: .failure(error)
        )
      }
    }
  }
  
  /// Cancel all fetching tasks for the given filter list
  func stopFetching(filterListCustomURL: FilterListCustomURL) async {
    let resource = await filterListCustomURL.setting.resource
    fetchTasks[resource]?.cancel()
    fetchTasks.removeValue(forKey: resource)
  }
}

extension CustomFilterListSetting {
  /// Return a download resource representing the given setting
  @MainActor var resource: FilterListCustomURLDownloader.DownloadResource {
    return FilterListCustomURLDownloader.DownloadResource(uuid: uuid, externalURL: externalURL)
  }
}
