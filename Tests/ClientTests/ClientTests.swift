/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import XCTest

import Shared
import Storage
import WebKit
import BraveShared
@testable import Brave

class ClientTests: XCTestCase {
  override func setUpWithError() throws {
    super.setUp()
    
    // TODO: Move this code into some module that is shared so it doesn't require AppDelegate/host application
    
    let server = WebServer.sharedInstance
    guard !server.server.isRunning else { return }
    
    let responders: [(String, InternalSchemeResponse)] =
    [
      (AboutHomeHandler.path, AboutHomeHandler()),
      (AboutLicenseHandler.path, AboutLicenseHandler()),
      (SessionRestoreHandler.path, SessionRestoreHandler()),
      (ErrorPageHandler.path, ErrorPageHandler()),
    ]
    responders.forEach { (path, responder) in
      InternalSchemeHandler.responders[path] = responder
    }
    
    ReaderModeHandlers.register(server, profile: BrowserProfile(localName: "test"))  // TODO: PORT TO InternalSchemeHandler
    BookmarksInterstitialPageHandler.register(server)  // TODO: PORT TO InternalSchemeHandler
    
    // Bug 1223009 was an issue whereby CGDWebserver crashed when moving to a background task
    // catching and handling the error seemed to fix things, but we're not sure why.
    // Either way, not implicitly unwrapping a try is not a great way of doing things
    // so this is better anyway.
    try server.start()
  }
  
  /// Our local server should only accept whitelisted hosts (localhost and 127.0.0.1).
  /// All other localhost equivalents should return 403.
  func testDisallowLocalhostAliases() throws {
    // Allowed local hosts. The first two are equivalent since iOS forwards an
    // empty host to localhost.
    [
      "localhost",
      "",
      "127.0.0.1",
    ].forEach { XCTAssert(hostIsValid($0), "\($0) host should be valid.") }

    // Disallowed local hosts. WKWebView will direct them to our server, but the server
    // should reject them.
    [
      "[::1]",
      "2130706433",
      "0",
      "127.00.00.01",
      "017700000001",
      "0x7f.0x0.0x0.0x1",
    ].forEach { XCTAssertFalse(hostIsValid($0), "\($0) host should not be valid.") }
  }

  func testDownloadsFolder() {
    let path = try? FileManager.default.downloadsPath()
    XCTAssertNotNil(path)

    XCTAssert(FileManager.default.fileExists(atPath: path!.path))

    // Let's pretend user deletes downloads folder via files.app
    XCTAssertNoThrow(try FileManager.default.removeItem(at: path!))

    XCTAssertFalse(FileManager.default.fileExists(atPath: path!.path))

    // Calling downloads path should recreate the deleted folder
    XCTAssertNoThrow(try FileManager.default.downloadsPath())

    XCTAssert(FileManager.default.fileExists(atPath: path!.path))
  }

  fileprivate func hostIsValid(_ host: String) -> Bool {
    let expectation = self.expectation(description: "Validate host for \(host)")
    let port = WebServer.sharedInstance.port

    var request = URLRequest(url: URL(string: "http://\(host):\(port)/reader-mode/page?url=https%3A%2F%2Fbrave.com")!)
    var response: HTTPURLResponse?

    let username = WebServer.sharedInstance.credentials.user ?? ""
    let password = WebServer.sharedInstance.credentials.password ?? ""

    let credentials = "\(username):\(password)".data(using: .utf8)?.base64EncodedString() ?? ""

    request.setValue("Basic \(credentials)", forHTTPHeaderField: "Authorization")

    URLSession(configuration: .ephemeral, delegate: nil, delegateQueue: .main).dataTask(with: request) { data, resp, error in
      response = resp as? HTTPURLResponse
      expectation.fulfill()
    }.resume()

    waitForExpectations(timeout: 200, handler: nil)
    return response?.statusCode == 200
  }
}
