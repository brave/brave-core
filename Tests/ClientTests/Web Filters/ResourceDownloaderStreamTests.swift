// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Brave

class ResourceDownloaderStreamTests: XCTestCase {
  func testSequenceWithSuccessfulDownload() throws {
    // Given
    let expectation = XCTestExpectation(description: "Test downloading resources")
    expectation.expectedFulfillmentCount = 2
    let resource = BraveS3Resource.adBlockRules
    let downloader = ResourceDownloader<BraveS3Resource>(networkManager: NetworkManager.makeNetworkManager(
      for: [resource], statusCode: 200
    ))
    
    let sequence = ResourceDownloaderStream(resource: resource, resourceDownloader: downloader, fetchInterval: 0.5)
    
    let task = Task { @MainActor in
      // When
      for try await result in sequence {
        self.ensureSuccessResult(result: result)
        expectation.fulfill()
      }
    }
    
    wait(for: [expectation], timeout: 10)
    task.cancel()
  }
  
  func testSequenceWithErrorDownload() throws {
    // Given
    let expectation = XCTestExpectation(description: "Test downloading resources")
    let resource = BraveS3Resource.adBlockRules
    let downloader = ResourceDownloader<BraveS3Resource>(networkManager: NetworkManager.makeNetworkManager(
      for: [resource], statusCode: 404
    ))
    
    let sequence = ResourceDownloaderStream(resource: resource, resourceDownloader: downloader, fetchInterval: 1)
    
    let task = Task { @MainActor in
      // When
      for try await result in sequence {
        self.ensureErrorResult(result: result)
        expectation.fulfill()
      }
    }
    
    wait(for: [expectation], timeout: 10)
    task.cancel()
  }
  
  @MainActor private func ensureSuccessResult(result: Result<ResourceDownloader<BraveS3Resource>.DownloadResult, Error>, file: StaticString = #filePath, line: UInt = #line) {
    // Then
    switch result {
    case .success:
      break
    case .failure(let error):
      XCTFail(error.localizedDescription, file: file, line: line)
    }
  }
  
  @MainActor private func ensureErrorResult(result: Result<ResourceDownloader<BraveS3Resource>.DownloadResult, Error>, file: StaticString = #filePath, line: UInt = #line) {
    // Then
    switch result {
    case .success:
      XCTFail("Should not be success", file: file, line: line)
    case .failure:
      break
    }
  }
}
