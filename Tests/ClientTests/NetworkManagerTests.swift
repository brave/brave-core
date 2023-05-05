// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Brave

class NetworkManagerTests: XCTestCase {

  var session: NetworkSessionMock!
  var manager: NetworkManager!
  var data: Data!
  var error: Error!

  let url = URL(string: "http://example.com")!
  let etag = "1234"

  override func setUp() {
    super.setUp()
    session = NetworkSessionMock()
    manager = NetworkManager(session: session)
    data = Data()
    error = NSError(domain: "example.com", code: 0, userInfo: nil)
  }

  func testDataRequestUrl() {
    let testResponse = getResponse()
    session.response = testResponse
    session.data = data
    session.error = nil

    let exp = XCTestExpectation(description: "testDataRequestUrl")
    Task {
      let (data, response) = try await self.manager.dataRequest(with: self.url)
      XCTAssertEqual(self.data, data)
      XCTAssertEqual(testResponse, response)
      exp.fulfill()
    }

    wait(for: [exp], timeout: 5)
    XCTAssert(true)
  }

  func testDataRequestUrlWithError() {
    let testResponse = getResponse()
    session.response = testResponse
    session.data = data
    session.error = error

    let exp = XCTestExpectation(description: "testDataRequestUrlWithError")
    Task {
      do {
        _ = try await self.manager.dataRequest(with: self.url)
      } catch {
        XCTAssertEqual((self.error as NSError).code, (error as NSError).code)
        XCTAssertEqual((self.error as NSError).domain, (error as NSError).domain)
        exp.fulfill()
      }
    }

    wait(for: [exp], timeout: 5)
    XCTAssert(true)
  }

  func testDataRequestUrlRequest() {
    let testResponse = getResponse()
    session.response = testResponse
    session.data = data
    session.error = nil

    let request = URLRequest(url: url)

    let exp = XCTestExpectation(description: "testDataRequestUrlRequest")
    Task {
      do {
        let (data, response) = try await self.manager.dataRequest(with: request)
        XCTAssertEqual(self.data, data)
        XCTAssertEqual(testResponse, response)
        exp.fulfill()
      } catch {

      }
    }

    wait(for: [exp], timeout: 5)
    XCTAssert(true)
  }

  func testDataRequestUrlRequestWithError() {
    let testResponse = getResponse()
    session.response = testResponse
    session.data = data
    session.error = error

    let request = URLRequest(url: url)

    let exp = XCTestExpectation(description: "testDataRequestUrlRequest")
    Task {
      do {
        _ = try await self.manager.dataRequest(with: request)
      } catch {
        XCTAssertEqual((self.error as NSError).code, (error as NSError).code)
        XCTAssertEqual((self.error as NSError).domain, (error as NSError).domain)
        exp.fulfill()
      }
    }

    wait(for: [exp], timeout: 5)
    XCTAssert(true)
  }

  func testDownloadResource() {
    let response = getResponse()
    session.data = data
    session.response = response

    let exp = XCTestExpectation(description: "testDownloadResource")
    Task {
      do {
        _ = try await self.manager.downloadResource(
          with: self.url,
          resourceType: .regular,
          retryTimeout: nil)
        exp.fulfill()
      } catch {

      }
    }

    wait(for: [exp], timeout: 5)
    XCTAssert(true)
  }

  func testDownloadResourceErrorCode() {
    let response = getResponse(statusCode: 404)
    session.data = data
    session.response = response

    let exp = XCTestExpectation(description: "testDownloadResourceErrorCode")

    Task {
      do {
        _ = try await self.manager.downloadResource(
          with: self.url,
          resourceType: .regular,
          retryTimeout: nil)
      } catch {
        exp.fulfill()
      }
    }

    wait(for: [exp], timeout: 5)
    XCTAssert(true)
  }

  func testDownloadResourceEmptyData() {
    let response = getResponse()
    session.data = nil
    session.response = response

    let exp = XCTestExpectation(description: "testDownloadResourceEmptyData")

    Task {
      do {
        let value = try await self.manager.downloadResource(
          with: self.url,
          resourceType: .regular,
          retryTimeout: nil)
        XCTAssert(value.data.isEmpty)
        exp.fulfill()
      } catch {

      }
    }

    wait(for: [exp], timeout: 5)
    XCTAssert(true)
  }

  func testDownloadResourceWithEtag() {
    let response = getResponse(headerFields: ["Etag": etag])
    session.data = data
    session.response = response

    let exp = XCTestExpectation(description: "testDownloadResource")
    Task {
      do {
        let value = try await self.manager.downloadResource(
          with: self.url,
          resourceType: .cached(etag: self.etag),
          retryTimeout: nil)
        XCTAssertEqual(value.etag, self.etag)
        exp.fulfill()
      } catch {

      }
    }

    wait(for: [exp], timeout: 5)
    XCTAssertNotNil(response.value(forHTTPHeaderField: "Etag"))
    XCTAssert(true)
  }

  func testDownloadResourceWithEmptyEtag() {
    let response = getResponse()
    session.data = data
    session.response = response

    let exp = XCTestExpectation(description: "testDownloadResource")
    Task {
      do {
        _ = try await self.manager.downloadResource(
          with: self.url,
          resourceType: .cached(etag: nil),
          retryTimeout: nil)
        exp.fulfill()
      } catch {

      }
    }

    wait(for: [exp], timeout: 5)

    // Can't really test more using this mock.
    XCTAssert(true)
  }

  func testDownloadResourceEtagExists() {
    let response = getResponse(statusCode: 304, headerFields: ["Etag": etag])
    session.data = data
    session.response = response

    let exp = XCTestExpectation(description: "testDownloadResource")
    exp.isInverted = true

    Task {
      do {
        _ = try await self.manager.downloadResource(
          with: self.url,
          resourceType: .cached(etag: self.etag),
          retryTimeout: nil)
        exp.fulfill()
      } catch {

      }
    }

    wait(for: [exp], timeout: 5)
    XCTAssert(true)
  }

  func testDownloadResourceTimeout() {
    let response = getResponse()
    session.data = data
    session.response = response
    session.error = error

    let exp = XCTestExpectation(description: "testDownloadResource")

    Task {
      do {
        _ = try await self.manager.downloadResource(
          with: self.url,
          resourceType: .regular,
          retryTimeout: 5)
        exp.fulfill()
      } catch {

      }
    }

    // Clear session's error before retry attempt
    session.error = nil
    wait(for: [exp], timeout: 2)
    XCTAssert(true)
  }

  private func getResponse(
    statusCode code: Int = 200,
    headerFields: [String: String]? = nil
  ) -> HTTPURLResponse {
    return HTTPURLResponse(
      url: url, statusCode: code,
      httpVersion: "HTTP/1.1", headerFields: headerFields)!
  }
}
