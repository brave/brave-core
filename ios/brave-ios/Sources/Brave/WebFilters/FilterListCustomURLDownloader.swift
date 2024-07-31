// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Combine
import Data
import Foundation

/// An actor that handles the downloading of custom filter lists which are sourced via a URL
@MainActor class FilterListCustomURLDownloader: ObservableObject {
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
  func startFetching() async {
    guard !startedService else { return }
    self.startedService = true
    await CustomFilterListStorage.shared.loadCachedFilterLists()

    for customURL in CustomFilterListStorage.shared.filterListsURLs {
      startFetching(filterListCustomURL: customURL)
    }
  }

  /// Perform a one time force update of all the filster lists
  func updateFilterLists() async throws {
    try await CustomFilterListStorage.shared.filterListsURLs.asyncForEach { customURL in
      let result = try await self.resourceDownloader.download(
        resource: customURL.setting.resource,
        force: true
      )
      self.handle(downloadResult: result, for: customURL)
    }
  }

  /// Handle the download results of a custom filter list. This will process the download by compiling iOS rule lists and adding the rule list to the `AdBlockEngineManager`.
  private func handle(
    downloadResult: ResourceDownloader<DownloadResource>.DownloadResult,
    for filterListCustomURL: FilterListCustomURL
  ) {
    let source = filterListCustomURL.setting.engineSource
    let version = downloadResult.version

    let fileInfo = AdBlockEngineManager.FileInfo(
      filterListInfo: GroupedAdBlockEngine.FilterListInfo(source: source, version: version),
      localFileURL: downloadResult.fileURL
    )

    AdBlockGroupsManager.shared.update(fileInfo: fileInfo)
    AdBlockGroupsManager.shared.compileEnginesIfFilesAreReady()
  }

  /// Start fetching the resource for the given filter list. Once a new version is downloaded, the file will be processed using the `handle` method
  func startFetching(filterListCustomURL: FilterListCustomURL) {
    guard startedService else { return }
    let resource = filterListCustomURL.setting.resource

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
            CustomFilterListStorage.shared.update(
              filterListId: filterListCustomURL.id,
              with: .success(downloadResult.date)
            )

            // Handle the successful result so we parse the content blockers
            self.handle(
              downloadResult: downloadResult,
              for: filterListCustomURL
            )
          case .failure(let error):
            // We don't want to keep refetching these types of failures
            let hardFailureCodes = [
              URLError.Code.badURL, .appTransportSecurityRequiresSecureConnection, .fileIsDirectory,
              .unsupportedURL,
            ]
            if let urlError = error as? URLError, hardFailureCodes.contains(urlError.code) {
              throw urlError
            }

            CustomFilterListStorage.shared.update(
              filterListId: filterListCustomURL.id,
              with: .failure(error)
            )
          }
        }
      } catch is CancellationError {
        self.fetchTasks.removeValue(forKey: resource)
      } catch {
        self.fetchTasks.removeValue(forKey: resource)

        CustomFilterListStorage.shared.update(
          filterListId: filterListCustomURL.id,
          with: .failure(error)
        )
      }
    }
  }

  /// Cancel all fetching tasks for the given filter list
  func stopFetching(filterListCustomURL: FilterListCustomURL) {
    let resource = filterListCustomURL.setting.resource
    fetchTasks[resource]?.cancel()
    fetchTasks.removeValue(forKey: resource)

    AdBlockGroupsManager.shared.removeFileInfo(
      for: filterListCustomURL.setting.engineSource
    )
    AdBlockGroupsManager.shared.compileEnginesIfFilesAreReady()
  }
}

extension CustomFilterListSetting {
  /// Return a download resource representing the given setting
  @MainActor var resource: FilterListCustomURLDownloader.DownloadResource {
    return FilterListCustomURLDownloader.DownloadResource(uuid: uuid, externalURL: externalURL)
  }
}
