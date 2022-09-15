// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// An endless sequence iterator for the given resource
final class ResourceDownloaderStream: Sendable, AsyncSequence, AsyncIteratorProtocol {
  /// An object representing the download
  struct DownloadResult: Equatable {
    let date: Date
    let fileURL: URL
  }
  
  typealias Element = Result<DownloadResult, Error>
  private let resource: ResourceDownloader.Resource
  private let resourceDownloader: ResourceDownloader
  private let fetchInterval: TimeInterval
  
  init(resource: ResourceDownloader.Resource, resourceDownloader: ResourceDownloader, fetchInterval: TimeInterval) {
    self.resource = resource
    self.resourceDownloader = resourceDownloader
    self.fetchInterval = fetchInterval
  }
  
  /// Returns the next downloaded value if it has changed since last time it was downloaded. Will return a cached result as an initial value.
  ///
  /// - Note: Only throws `CancellationError` error. Downloading errors are returned as a `Result` object
  func next() async throws -> Element? {
    // Keep fetching new data until we get a new result
    while true {
      try await Task.sleep(seconds: fetchInterval)
      
      do {
        let result = try await resourceDownloader.download(resource: resource)
        
        switch result {
        case .downloaded(let url, let date):
          return .success(DownloadResult(date: date, fileURL: url))
        case .notModified:
          continue
        }
      } catch {
        return .failure(error)
      }
    }
  }
  
  func makeAsyncIterator() -> ResourceDownloaderStream {
    return self
  }
}
