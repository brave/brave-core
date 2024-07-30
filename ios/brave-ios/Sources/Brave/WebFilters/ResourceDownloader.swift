// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Shared

/// A ganeric resource downloader class that is responsible for fetching resources
actor ResourceDownloader<Resource: DownloadResourceInterface>: Sendable {
  /// An object representing the download
  struct DownloadResult: Equatable {
    let date: Date
    let fileURL: URL
    let isModified: Bool
  }

  /// An object representing errors during a resource download
  enum DownloadResultError: Error {
    case noData
  }

  /// An object represening the download result
  private enum DownloadResultStatus {
    case notModified(URL, Date)
    case downloaded(CachedNetworkResource, Date)
  }

  /// The default fetch interval used by this resource downloaded. In production its 6 hours, whereas in debug it's every 10 minutes.
  private static var defaultFetchInterval: TimeInterval {
    return AppConstants.isOfficialBuild ? 6.hours : 10.minutes
  }

  /// The netowrk manager performing the requests
  private let networkManager: NetworkManager

  /// Initialize this class with the given network manager
  init(networkManager: NetworkManager = NetworkManager()) {
    self.networkManager = networkManager
  }

  /// Return a download stream for the given resource. The download stream will fetch data every interval given by the provided `fetchInterval`.
  func downloadStream(
    for resource: Resource,
    every fetchInterval: TimeInterval = defaultFetchInterval
  ) -> ResourceDownloaderStream<Resource> {
    return ResourceDownloaderStream(
      resource: resource,
      resourceDownloader: self,
      fetchInterval: fetchInterval
    )
  }

  /// Download the give resource type for the filter list and store it into the cache folder url
  @discardableResult
  func download(resource: Resource, force: Bool = false) async throws -> DownloadResult {
    let result = try await downloadInternal(resource: resource, force: force)

    switch result {
    case .downloaded(let networkResource, let date):
      // Clear any old data
      try await resource.removeFile()
      // Make a cache folder if needed
      let cacheFolderURL = try await resource.getOrCreateCacheFolder()
      // Save the data to file
      let fileURL = cacheFolderURL.appendingPathComponent(resource.cacheFileName)
      try writeDataToDisk(data: networkResource.data, toFileURL: fileURL)
      // Save the etag to file
      if let data = networkResource.etag?.data(using: .utf8) {
        try writeDataToDisk(
          data: data,
          toFileURL: cacheFolderURL.appendingPathComponent(resource.etagFileName)
        )
      }
      // Return the file URL
      let creationDate = try? await resource.creationDate()
      return DownloadResult(
        date: creationDate ?? date,
        fileURL: fileURL,
        isModified: true
      )
    case .notModified(let fileURL, let date):
      let creationDate = try? await resource.creationDate()
      return DownloadResult(
        date: creationDate ?? date,
        fileURL: fileURL,
        isModified: false
      )
    }
  }

  private func downloadInternal(
    resource: Resource,
    force: Bool = false
  ) async throws -> DownloadResultStatus {
    let etag = force ? nil : try? await resource.createdEtag()

    do {
      let networkResource = try await self.networkManager.downloadResource(
        with: resource.externalURL,
        resourceType: .cached(etag: etag),
        checkLastServerSideModification: !AppConstants.isOfficialBuild,
        customHeaders: resource.headers
      )

      guard !networkResource.data.isEmpty else {
        throw DownloadResultError.noData
      }

      let date = try await resource.creationDate()
      return .downloaded(networkResource, date ?? Date())
    } catch let error as NetworkManagerError {
      if error == .fileNotModified, let fileURL = resource.downloadedFileURL {
        let date = try await resource.creationDate()
        return .notModified(fileURL, date ?? Date())
      } else {
        throw error
      }
    }
  }

  /// Write the given `Data` to disk into to the specified file `URL`
  /// into the `applicationSupportDirectory` `SearchPathDirectory`.
  ///
  /// - Note: `fileName` must contain the full file name including the extension.
  private func writeDataToDisk(data: Data, toFileURL fileURL: URL) throws {
    try data.write(to: fileURL, options: [.atomic])
  }
}
