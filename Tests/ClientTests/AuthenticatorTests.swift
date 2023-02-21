/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import XCTest
import Shared
import BraveSharedTestUtils
@testable import Brave
@testable import Storage

class MockFiles: FileAccessor {
  init() {
    let docPath = NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory, .userDomainMask, true)[0]
    super.init(rootPath: (docPath as NSString).appendingPathComponent("testing"))
  }
}

class MockChallengeSender: NSObject, URLAuthenticationChallengeSender {
  func use(_ credential: URLCredential, for challenge: URLAuthenticationChallenge) {}
  func continueWithoutCredential(for challenge: URLAuthenticationChallenge) {}
  func cancel(_ challenge: URLAuthenticationChallenge) {}
}

class MockMalformableLogin: LoginData {
  var guid: String
  var credentials: URLCredential
  var protectionSpace: URLProtectionSpace
  var hostname: String
  var username: String?
  var password: String
  var httpRealm: String?
  var formSubmitURL: String?
  var usernameField: String?
  var passwordField: String?
  var hasMalformedHostname = true
  
  func validate() throws { }

  static func createWithHostname(_ hostname: String, username: String, password: String, formSubmitURL: String) -> MockMalformableLogin {
    return self.init(guid: Bytes.generateGUID(), hostname: hostname, username: username, password: password, formSubmitURL: formSubmitURL)
  }

  required init(guid: String, hostname: String, username: String, password: String, formSubmitURL: String) {
    self.guid = guid
    self.credentials = URLCredential(user: username, password: password, persistence: .none)
    self.hostname = hostname
    self.password = password
    self.username = username
    self.protectionSpace = URLProtectionSpace(host: hostname, port: 0, protocol: nil, realm: nil, authenticationMethod: nil)
    self.formSubmitURL = formSubmitURL
  }

  func toDict() -> [String: String] {
    // Not used for this mock
    return [String: String]()
  }

  func isSignificantlyDifferentFrom(_ login: LoginData) -> Bool {
    // Not used for this mock
    return true
  }

  func update(password: String, username: String) {
    // Not used for this mock
  }
}

private let MainLoginColumns = "guid, username, password, hostname, httpRealm, formSubmitURL, usernameField, passwordField"

class AuthenticatorTests: XCTestCase {

  fileprivate var db: BrowserDB!
  fileprivate var logins: SQLiteLogins!
  fileprivate var mockVC = UIViewController()

  override func setUp() {
    super.setUp()
    self.db = BrowserDB(filename: "testsqlitelogins.db", schema: LoginsSchema(), files: MockFiles())
    self.logins = SQLiteLogins(db: self.db)
    let expectation = expectation(description: "setUp")
    Task { @MainActor in
      await XCTAssertAsyncNoThrow(try await self.logins.removeAll())
      expectation.fulfill()
    }
    waitForExpectations(timeout: 10.0)
  }

  override func tearDown() {
    let expectation = expectation(description: "tearDown")
    Task { @MainActor in
      await XCTAssertAsyncNoThrow(try await self.logins.removeAll())
      expectation.fulfill()
    }
    waitForExpectations(timeout: 10.0)
  }

  fileprivate func mockChallengeForURL(_ url: URL, username: String, password: String) -> URLAuthenticationChallenge {
    let scheme = url.scheme
    let host = url.host ?? ""
    let port = (url as NSURL).port?.intValue ?? 80

    let credential = URLCredential(user: username, password: password, persistence: .none)
    let protectionSpace = URLProtectionSpace(
      host: host,
      port: port,
      protocol: scheme,
      realm: "Secure Site",
      authenticationMethod: nil)
    return URLAuthenticationChallenge(
      protectionSpace: protectionSpace,
      proposedCredential: credential,
      previousFailureCount: 0,
      failureResponse: nil,
      error: nil,
      sender: MockChallengeSender())
  }

  fileprivate func hostnameFactory(_ row: SDRow) -> String {
    return row["hostname"] as! String
  }

  fileprivate func rawQueryForAllLogins() async throws -> Cursor<String> {
    let projection = MainLoginColumns
    let sql = """
      SELECT \(projection)
      FROM loginsL
      WHERE is_deleted = 0
      UNION ALL
      SELECT \(projection)
      FROM loginsM
      WHERE is_overridden = 0
      ORDER BY hostname ASC
      """
    return try await db.runQuery(sql, args: nil, factory: hostnameFactory)
  }

  func testChallengeMatchesLoginEntry() async throws {
    let login = Login.createWithHostname("https://securesite.com", username: "username", password: "password", formSubmitURL: "https://submit.me")
    try await logins.addLogin(login)
    let challenge = mockChallengeForURL(URL(string: "https://securesite.com")!, username: "username", password: "password")
    let result = await Authenticator.findMatchingCredentialsForChallenge(challenge.protectionSpace, fromLoginsProvider: logins)
    XCTAssertNotNil(result)
    XCTAssertEqual(result?.user, "username")
    XCTAssertEqual(result?.password, "password")
  }

  func testChallengeMatchesSingleMalformedLoginEntry() async throws {
    // Since Login has been updated to not store schemeless URL, write directly to simulate a malformed URL
    let malformedLogin = MockMalformableLogin.createWithHostname("malformed.com", username: "username", password: "password", formSubmitURL: "https://submit.me")
    try await logins.addLogin(malformedLogin)

    // Pre-condition: Check that the hostname is malformed
    let oldHostname = try await rawQueryForAllLogins()[0]
    XCTAssertEqual(oldHostname, "malformed.com")

    let challenge = mockChallengeForURL(URL(string: "https://malformed.com")!, username: "username", password: "password")
    let result = await Authenticator.findMatchingCredentialsForChallenge(challenge.protectionSpace, fromLoginsProvider: logins)
    XCTAssertNotNil(result)
    XCTAssertEqual(result?.user, "username")
    XCTAssertEqual(result?.password, "password")

    // Post-condition: Check that we updated the hostname to be not malformed
    let newHostname = try await rawQueryForAllLogins()[0]
    XCTAssertEqual(newHostname, "https://malformed.com")
  }

  func testChallengeMatchesDuplicateLoginEntries() async throws {
    // Since Login has been updated to not store schemeless URL, write directly to simulate a malformed URL
    let malformedLogin = MockMalformableLogin.createWithHostname("malformed.com", username: "malformed_username", password: "malformed_password", formSubmitURL: "https://submit.me")
    try await logins.addLogin(malformedLogin)

    let login = Login.createWithHostname("https://malformed.com", username: "good_username", password: "good_password", formSubmitURL: "https://submit.me")
    try await logins.addLogin(login)

    // Pre-condition: Verify that both logins were stored
    let hostnames = try await rawQueryForAllLogins()
    XCTAssertEqual(hostnames.count, 2)
    XCTAssertEqual(hostnames[0], "https://malformed.com")
    XCTAssertEqual(hostnames[1], "malformed.com")

    let challenge = mockChallengeForURL(URL(string: "https://malformed.com")!, username: "username", password: "password")
    let result = await Authenticator.findMatchingCredentialsForChallenge(challenge.protectionSpace, fromLoginsProvider: logins)
    XCTAssertNotNil(result)
    XCTAssertEqual(result?.user, "good_username")
    XCTAssertEqual(result?.password, "good_password")

    // Post-condition: Verify that malformed URL was removed
    let newHostnames = try await rawQueryForAllLogins()
    XCTAssertEqual(newHostnames.count, 1)
    XCTAssertEqual(newHostnames[0], "https://malformed.com")
  }
}
