// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// An endless sequence iterator for the given resource
struct ResourceDownloaderStream: Sendable, AsyncSequence, AsyncIteratorProtocol {
  /// An object representing the download
  struct DownloadResult: Equatable {
    let date: Date
    let fileURL: URL
    let isModified: Bool
  }
  
  typealias Element = Result<DownloadResult, Error>
  private let resource: ResourceDownloader.Resource
  private let resourceDownloader: ResourceDownloader
  private let fetchInterval: TimeInterval
  private var firstLoad = true
  
  init(resource: ResourceDownloader.Resource, resourceDownloader: ResourceDownloader, fetchInterval: TimeInterval) {
    self.resource = resource
    self.resourceDownloader = resourceDownloader
    self.fetchInterval = fetchInterval
  }
  
  /// Returns the next downloaded value if it has changed since last time it was downloaded. Will return a cached result as an initial value.
  ///
  /// - Note: Only throws `CancellationError` error. Downloading errors are returned as a `Result` object
  mutating func next() async throws -> Element? {
    if firstLoad {
      // On a first load, we return the result so that they are available right away.
      // After that we wait only for changes while sleeping
      do {
        self.firstLoad = false
        let result = try await result()
        return .success(result)
      } catch let error as URLError {
        // Soft fail these errors
        return .failure(error)
      } catch {
        throw error
      }
    }
    
    // Keep fetching new data until we get a new result
    while true {
      try await Task.sleep(seconds: fetchInterval)
      
      do {
        let result = try await result()
        guard result.isModified else { continue }
        return .success(result)
      } catch let error as URLError {
        // Soft fail these errors
        return .failure(error)
      } catch {
        throw error
      }
    }
  }
  
  private func result() async throws -> DownloadResult {
    switch try await resourceDownloader.download(resource: resource) {
    case .downloaded(let url, let date):
      return DownloadResult(date: date, fileURL: url, isModified: true)
    case .notModified(let url, let date):
      return DownloadResult(date: date, fileURL: url, isModified: false)
    }
  }
  
  func makeAsyncIterator() -> ResourceDownloaderStream {
    return self
  }
  
  static func cachedResult(for resource: ResourceDownloader.Resource) throws -> DownloadResult? {
    guard let fileURL = ResourceDownloader.downloadedFileURL(for: resource) else { return nil }
    guard let creationDate = try ResourceDownloader.creationDate(for: resource) else { return nil }
    return DownloadResult(date: creationDate, fileURL: fileURL, isModified: false)
  }
}
