// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Client

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
        session.error = error
        
        manager.dataRequest(with: url) { data, response, error in
            XCTAssertEqual(self.data, data)
            XCTAssertEqual(testResponse, response)
            XCTAssertEqual(self.error.debugDescription, error.debugDescription)
        }
    }
    
    func testDataRequestUrlRequest() {
        let testResponse = getResponse()
        session.response = testResponse
        session.data = data
        session.error = error
        
        let request = URLRequest(url: url)
        
        manager.dataRequest(with: request) { data, response, error in
            XCTAssertEqual(self.data, data)
            XCTAssertEqual(testResponse, response)
            XCTAssertEqual(self.error.debugDescription, error.debugDescription)
        }
    }
    
    func testDownloadResource() {
        let response = getResponse()
        session.data = data
        session.response = response
        
        let exp = XCTestExpectation(description: "testDownloadResource")
        let completion = manager.downloadResource(with: url, resourceType: .regular, retryTimeout: nil)
        
        completion.upon { _ in exp.fulfill() }
        wait(for: [exp], timeout: 1)
        
        XCTAssert(true)
    }

    func testDownloadResourceErrorCode() {
        let response = getResponse(statusCode: 404)
        session.data = data
        session.response = response
        
        let exp = XCTestExpectation(description: "testDownloadResourceErrorCode")
        exp.isInverted = true
        
        let completion = manager.downloadResource(with: url, resourceType: .regular, retryTimeout: nil)
        completion.upon { _ in exp.fulfill() }
        
        wait(for: [exp], timeout: 1)
        
        XCTAssert(true)
    }
    
    func testDownloadResourceEmptyData() {
        let response = getResponse()
        session.data = nil
        session.response = response
        
        let exp = XCTestExpectation(description: "testDownloadResourceEmptyData")
        exp.isInverted = true
        
        let completion = manager.downloadResource(with: url, resourceType: .regular, retryTimeout: nil)
        completion.upon { _ in exp.fulfill() }
        
        wait(for: [exp], timeout: 1)
        
        XCTAssert(true)
    }
    
    func testDownloadResourceWithEtag() {
        let response = getResponse(headerFields: ["Etag": etag])
        session.data = data
        session.response = response
        
        let exp = XCTestExpectation(description: "testDownloadResource")
        let completion = manager.downloadResource(with: url, resourceType: .cached(etag: etag),
                                                  retryTimeout: nil)
        
        completion.upon { _ in exp.fulfill() }
        wait(for: [exp], timeout: 1)
        
        XCTAssertEqual(completion.value.etag, etag)
        XCTAssertNotNil(response.allHeaderFields["Etag"])
        
        XCTAssert(true)
    }
    
    func testDownloadResourceWithEmptyEtag() {
        let response = getResponse()
        session.data = data
        session.response = response
        
        let exp = XCTestExpectation(description: "testDownloadResource")
        let completion = manager.downloadResource(with: url, resourceType: .cached(etag: nil),
                                                  retryTimeout: nil)
        
        completion.upon { _ in exp.fulfill() }
        wait(for: [exp], timeout: 1)
        
        // Can't really test more using this mock.
        XCTAssert(true)
    }
    
    func testDownloadResourceEtagExists() {
        let response = getResponse(statusCode: 304, headerFields: ["Etag": etag])
        session.data = data
        session.response = response
        
        let exp = XCTestExpectation(description: "testDownloadResource")
        exp.isInverted = true
        let completion = manager.downloadResource(with: url, resourceType: .cached(etag: etag),
                                                  retryTimeout: nil)
        
        completion.upon { _ in exp.fulfill() }
        wait(for: [exp], timeout: 1)
        
        XCTAssert(true)
    }
    
    func testDownloadResourceTimeout() {
        let response = getResponse()
        session.data = data
        session.response = response
        session.error = error

        
        let exp = XCTestExpectation(description: "testDownloadResource")
        let completion = manager.downloadResource(with: url, resourceType: .regular, retryTimeout: 1)
        // Clear session's error before retry attempt
        session.error = nil
        
        completion.upon { _ in
            exp.fulfill()
        }
        wait(for: [exp], timeout: 2)
        
        XCTAssert(true)
    }
    
    private func getResponse(statusCode code: Int = 200,
                             headerFields: [String : String]? = nil) -> HTTPURLResponse {
        return HTTPURLResponse(url: url, statusCode: code,
                               httpVersion: "HTTP/1.1", headerFields: headerFields)!
    }
}
