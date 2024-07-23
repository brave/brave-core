// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import Brave

class ResourceDownloaderTests: XCTestCase {
  func testSuccessfulResourceDownload() throws {
    // Given
    let expectation = XCTestExpectation(description: "Test downloading resources")
    let resource = BraveS3Resource.slimList
    let firstDownloader = ResourceDownloader<BraveS3Resource>(
      networkManager: NetworkManager.makeNetworkManager(
        for: [resource],
        statusCode: 200,
        etag: "123"
      )
    )
    let secondDownloader = ResourceDownloader<BraveS3Resource>(
      networkManager: NetworkManager.makeNetworkManager(
        for: [resource],
        statusCode: 304,
        etag: "123"
      )
    )

    Task {
      do {
        // When
        // We do first download
        let result = try await firstDownloader.download(resource: resource)

        // Then
        // We get a download result
        let downloadedData = try await resource.downloadedData()
        let createdETag = try await resource.createdEtag()
        XCTAssertNotNil(downloadedData)
        XCTAssertNotNil(createdETag)
        XCTAssertTrue(result.isModified)

        // When
        // We re-download
        let result2 = try await secondDownloader.download(resource: resource)

        // Then
        // We get a non modified result
        let downloadedData2 = try await resource.downloadedData()
        let createdETag2 = try await resource.createdEtag()
        XCTAssertNotNil(downloadedData2)
        XCTAssertNotNil(createdETag2)
        XCTAssertFalse(result2.isModified)
        // Same download date
        XCTAssertEqual(result2.date, result.date)
      } catch {
        XCTFail(error.localizedDescription)
      }

      expectation.fulfill()
    }

    wait(for: [expectation], timeout: 5)
  }

  func testFailedResourceDownload() throws {
    // Given
    let expectation = XCTestExpectation(description: "Test downloading resource")
    let resource = BraveS3Resource.slimList
    let downloader = ResourceDownloader<BraveS3Resource>(
      networkManager: NetworkManager.makeNetworkManager(
        for: [resource],
        statusCode: 404
      )
    )

    Task {
      do {
        // When
        _ = try await downloader.download(resource: resource)
        XCTFail("Should not succeed")
      } catch {
        // Then
        // We get an error
      }

      expectation.fulfill()
    }

    wait(for: [expectation], timeout: 5)
  }
}
