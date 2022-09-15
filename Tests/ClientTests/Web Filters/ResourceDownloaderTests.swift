// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Brave

class ResourceDownloaderTests: XCTestCase {
  func testSuccessfulResourceDownload() throws {
    // Given
    let expectation = XCTestExpectation(description: "Test downloading resources")
    let resource = ResourceDownloader.Resource.debounceRules
    let firstDownloader = ResourceDownloader(networkManager: NetworkManager.makeNetworkManager(
      for: [resource], statusCode: 200, etag: "123"
    ))
    let secondDownloader = ResourceDownloader(networkManager: NetworkManager.makeNetworkManager(
      for: [resource], statusCode: 304, etag: "123"
    ))
    
    Task {
      do {
        // When
        let result = try await firstDownloader.download(resource: resource)
        
        // Then
        // We get a download result
        switch result {
        case .downloaded:
          XCTAssertNotNil(try ResourceDownloader.data(for: resource))
          XCTAssertNotNil(try ResourceDownloader.etag(for: resource))
        case .notModified:
          XCTFail("Not modified recieved")
        }
      } catch {
        XCTFail(error.localizedDescription)
      }
      
      do {
        // When
        let result = try await secondDownloader.download(resource: resource)
        
        // Then
        // We get a not modified result
        switch result {
        case .downloaded:
          XCTFail("Not modified recieved")
        case .notModified:
          XCTAssertNotNil(try ResourceDownloader.data(for: resource))
          XCTAssertNotNil(try ResourceDownloader.etag(for: resource))
        }
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
    let resource = ResourceDownloader.Resource.debounceRules
    let downloader = ResourceDownloader(networkManager: NetworkManager.makeNetworkManager(
      for: [resource], statusCode: 404
    ))
    
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
