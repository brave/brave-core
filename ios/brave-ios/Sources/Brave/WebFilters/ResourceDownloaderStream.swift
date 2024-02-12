// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// An endless sequence iterator for the given resource
struct ResourceDownloaderStream<Resource: DownloadResourceInterface>: Sendable, AsyncSequence, AsyncIteratorProtocol {
  typealias Element = Result<ResourceDownloader<Resource>.DownloadResult, Error>
  private let resource: Resource
  private let resourceDownloader: ResourceDownloader<Resource>
  private let fetchInterval: TimeInterval
  private var firstLoad = true
  
  init(resource: Resource, resourceDownloader: ResourceDownloader<Resource>, fetchInterval: TimeInterval) {
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
        let result = try await resourceDownloader.download(resource: resource)
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
        let result = try await resourceDownloader.download(resource: resource)
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
  
  func makeAsyncIterator() -> ResourceDownloaderStream {
    return self
  }
}
