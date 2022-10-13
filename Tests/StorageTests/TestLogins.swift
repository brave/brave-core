/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
@testable import Storage
import BraveSharedTestUtils
import XCTest

class TestSQLiteLogins: XCTestCase {
  var db: BrowserDB!
  var logins: SQLiteLogins!

  let formSubmitURL = "http://submit.me"
  let login = Login.createWithHostname("hostname1", username: "username1", password: "password1", formSubmitURL: "http://submit.me")

  override func setUp() {
    super.setUp()

    let files = MockFiles()
    self.db = BrowserDB(filename: "testsqlitelogins.db", schema: LoginsSchema(), files: files)
    self.logins = SQLiteLogins(db: self.db)

    let expectation = self.expectation(description: "Remove all logins.")
    Task { @MainActor in
      try await self.removeAllLogins()
      expectation.fulfill()
    }
    waitForExpectations(timeout: 10.0, handler: nil)
  }

  func testAddLogin() async throws {
    try await addLogin(login)
    try await getLoginsFor(login.protectionSpace, expected: [login])()
  }

  func testGetOrder() async throws {
    // Different GUID.
    let login2 = Login.createWithHostname("hostname1", username: "username2", password: "password2")
    login2.formSubmitURL = "http://submit.me"

    try await addLogin(login)
    try await addLogin(login2)
    try await getLoginsFor(login.protectionSpace, expected: [login2, login])()
  }

  func testRemoveLogin() async throws {
    try await addLogin(login)
    try await self.removeLogin(self.login)
    try await getLoginsFor(login.protectionSpace, expected: [])()
  }

  func testRemoveLogins() async throws {
    let loginA = Login.createWithHostname("alphabet.com", username: "username1", password: "password1", formSubmitURL: formSubmitURL)
    let loginB = Login.createWithHostname("alpha.com", username: "username2", password: "password2", formSubmitURL: formSubmitURL)
    let loginC = Login.createWithHostname("berry.com", username: "username3", password: "password3", formSubmitURL: formSubmitURL)
    let loginD = Login.createWithHostname("candle.com", username: "username4", password: "password4", formSubmitURL: formSubmitURL)

    func addLogins() async throws {
      _ = try await addLogin(loginA)
      _ = try await addLogin(loginB)
      _ = try await addLogin(loginC)
      _ = try await addLogin(loginD)
    }

    try await addLogins()
    let guids = [loginA.guid, loginB.guid]
    try await logins.removeLoginsWithGUIDs(guids)
    let result = try await logins.getAllLogins()
    XCTAssertEqual(result.count, 2)
  }

  func testRemoveManyLogins() async throws {
    var guids: [GUID] = []
    for i in 0..<2000 {
      let login = Login.createWithHostname("mozilla.org", username: "Fire", password: "fox", formSubmitURL: formSubmitURL)
      if i <= 1000 {
        guids += [login.guid]
      }
      try await addLogin(login)
    }
    try await logins.removeLoginsWithGUIDs(guids)
    let result = try await logins.getAllLogins()
    XCTAssertEqual(result.count, 999)
  }

  func testUpdateLogin() async throws {
    let updated = Login.createWithHostname("hostname1", username: "username1", password: "password3", formSubmitURL: formSubmitURL)
    updated.guid = self.login.guid

    try await addLogin(login)
    try await updateLogin(updated)
    try await getLoginsFor(login.protectionSpace, expected: [updated])()
  }

  func testAddInvalidLogin() async throws {
    let emptyPasswordLogin = Login.createWithHostname("hostname1", username: "username1", password: "", formSubmitURL: formSubmitURL)
    await XCTAssertAsyncThrowsError(try await logins.addLogin(emptyPasswordLogin))

    let emptyHostnameLogin = Login.createWithHostname("", username: "username1", password: "password", formSubmitURL: formSubmitURL)
    await XCTAssertAsyncThrowsError(try await logins.addLogin(emptyHostnameLogin))

    let credential = URLCredential(user: "username", password: "password", persistence: .forSession)
    let protectionSpace = URLProtectionSpace(host: "https://website.com", port: 443, protocol: "https", realm: "Basic Auth", authenticationMethod: "Basic Auth")
    let bothFormSubmitURLAndRealm = Login.createWithCredential(credential, protectionSpace: protectionSpace)
    bothFormSubmitURLAndRealm.formSubmitURL = "http://submit.me"
    await XCTAssertAsyncThrowsError(try await logins.addLogin(bothFormSubmitURLAndRealm))

    let noFormSubmitURLOrRealm = Login.createWithHostname("host", username: "username1", password: "password", formSubmitURL: nil)
    await XCTAssertAsyncThrowsError(try await logins.addLogin(noFormSubmitURLOrRealm))
  }

  func testUpdateInvalidLogin() async throws {
    let updated = Login.createWithHostname("hostname1", username: "username1", password: "", formSubmitURL: formSubmitURL)
    updated.guid = self.login.guid

    try await addLogin(login)
    await XCTAssertAsyncThrowsError(try await logins.updateLoginByGUID(login.guid, new: updated, significant: true))

    let emptyHostnameLogin = Login.createWithHostname("", username: "username1", password: "", formSubmitURL: formSubmitURL)
    emptyHostnameLogin.guid = self.login.guid
    await XCTAssertAsyncThrowsError(try await logins.updateLoginByGUID(login.guid, new: emptyHostnameLogin, significant: true))

    let credential = URLCredential(user: "username", password: "password", persistence: .forSession)
    let protectionSpace = URLProtectionSpace(host: "https://website.com", port: 443, protocol: "https", realm: "Basic Auth", authenticationMethod: "Basic Auth")
    let bothFormSubmitURLAndRealm = Login.createWithCredential(credential, protectionSpace: protectionSpace)
    bothFormSubmitURLAndRealm.formSubmitURL = "http://submit.me"
    bothFormSubmitURLAndRealm.guid = self.login.guid
    await XCTAssertAsyncThrowsError(try await logins.updateLoginByGUID(login.guid, new: bothFormSubmitURLAndRealm, significant: true))

    let noFormSubmitURLOrRealm = Login.createWithHostname("host", username: "username1", password: "password", formSubmitURL: nil)
    noFormSubmitURLOrRealm.guid = self.login.guid
    await XCTAssertAsyncThrowsError(try await logins.updateLoginByGUID(login.guid, new: noFormSubmitURLOrRealm, significant: true))
  }

  func testSearchLogins() async {
    let loginA = Login.createWithHostname("alphabet.com", username: "username1", password: "password1", formSubmitURL: formSubmitURL)
    let loginB = Login.createWithHostname("alpha.com", username: "username2", password: "password2", formSubmitURL: formSubmitURL)
    let loginC = Login.createWithHostname("berry.com", username: "username3", password: "password3", formSubmitURL: formSubmitURL)
    let loginD = Login.createWithHostname("candle.com", username: "username4", password: "password4", formSubmitURL: formSubmitURL)

    func addLogins() async throws {
      _ = try await addLogin(loginA)
      _ = try await addLogin(loginB)
      _ = try await addLogin(loginC)
      _ = try await addLogin(loginD)
    }

    func checkAllLogins() async throws {
      let results = try await logins.getAllLogins()
      XCTAssertEqual(results.count, 4)
    }
    
    await XCTAssertAsyncNoThrow(try await addLogins())
    await XCTAssertAsyncNoThrow(try await checkAllLogins())
    await XCTAssertAsyncNoThrow(try await removeAllLogins())
  }

  /*
    func testAddUseOfLogin() {
        let expectation = self.self.expectation(description: "Add visit")

        if var usageData = login as? LoginUsageData {
            usageData.timeCreated = Date.nowMicroseconds()
        }

        addLogin(login) >>>
            addUseDelayed(login, time: 1) >>>
            getLoginDetailsFor(login, expected: login as! LoginUsageData) >>>
            done(login.protectionSpace, expectation: expectation)

        waitForExpectations(timeout: 10.0, handler: nil)
    }
    */

  // Note: These functions are all curried so that we pass arguments, but still chain them below
  func addLogin(_ login: LoginData) async throws {
    return try await logins.addLogin(login)
  }

  func updateLogin(_ login: LoginData) async throws {
    return try await logins.updateLoginByGUID(login.guid, new: login, significant: true)
  }

  func addUseDelayed(_ login: Login, time: UInt32) async throws {
    sleep(time)
    login.timeLastUsed = Date.nowMicroseconds()
    try await logins.addUseOfLoginByGUID(login.guid)
    sleep(time)
  }

  func getLoginsFor(_ protectionSpace: URLProtectionSpace, expected: [LoginData]) -> (() async throws -> Void) {
    return {
      let results = try await self.logins.getLoginsForProtectionSpace(protectionSpace)
      XCTAssertEqual(expected.count, results.count)
      for (index, login) in expected.enumerated() {
        XCTAssertEqual(results[index]!.username!, login.username!)
        XCTAssertEqual(results[index]!.hostname, login.hostname)
        XCTAssertEqual(results[index]!.password, login.password)
      }
    }
  }

  func removeLogin(_ login: LoginData) async throws {
    return try await logins.removeLoginByGUID(login.guid)
  }

  func removeAllLogins() async throws {
    // Because we don't want to just mark them as deleted.
    try await self.db.run("DELETE FROM loginsM")
    try await self.db.run("DELETE FROM loginsL")
  }
}

class TestSQLiteLoginsPerf: XCTestCase {
  var db: BrowserDB!
  var logins: SQLiteLogins!

  override func setUp() {
    super.setUp()
    let files = MockFiles()
    self.db = BrowserDB(filename: "testsqlitelogins.db", schema: LoginsSchema(), files: files)
    self.logins = SQLiteLogins(db: self.db)
  }

  func testLoginsSearchMatchOnePerf() async throws {
    try await populateTestLogins()

    await XCTAssertAsyncNoThrow(try await removeAllLogins())
  }

  func testLoginsSearchMatchAllPerf() async throws {
    try await populateTestLogins()

    await XCTAssertAsyncNoThrow(try await removeAllLogins())
  }

  func populateTestLogins() async throws {
    for i in 0..<1000 {
      let login = Login.createWithHostname("website\(i).com", username: "username\(i)", password: "password\(i)", formSubmitURL: "test")
      try await addLogin(login)
    }
  }

  func addLogin(_ login: LoginData) async throws {
    return try await logins.addLogin(login)
  }

  func removeAllLogins() async throws {
    // Because we don't want to just mark them as deleted.
    try await self.db.run("DELETE FROM loginsM")
    try await self.db.run("DELETE FROM loginsL")
  }
}

class TestSyncableLogins: XCTestCase {
  var db: BrowserDB!
  var logins: SQLiteLogins!

  override func setUp() {
    super.setUp()

    let files = MockFiles()
    self.db = BrowserDB(filename: "testsyncablelogins.db", schema: LoginsSchema(), files: files)
    self.logins = SQLiteLogins(db: self.db)

    let expectation = self.expectation(description: "Remove all logins.")
    Task { @MainActor in
      try await self.removeAllLogins()
      expectation.fulfill()
    }
    waitForExpectations(timeout: 10.0, handler: nil)
  }

  func removeAllLogins() async throws {
    // Because we don't want to just mark them as deleted.
    try await self.db.run("DELETE FROM loginsM")
    try await self.db.run("DELETE FROM loginsL")
  }

  func testDiffers() {
    let guid = "abcdabcdabcd"
    let host = "http://example.com"
    let user = "username"
    let loginA1 = Login(guid: guid, hostname: host, username: user, password: "password1")
    loginA1.formSubmitURL = "\(host)/form1/"
    loginA1.usernameField = "afield"

    let loginA2 = Login(guid: guid, hostname: host, username: user, password: "password1")
    loginA2.formSubmitURL = "\(host)/form1/"
    loginA2.usernameField = "somefield"

    let loginB = Login(guid: guid, hostname: host, username: user, password: "password2")
    loginB.formSubmitURL = "\(host)/form1/"

    let loginC = Login(guid: guid, hostname: host, username: user, password: "password")
    loginC.formSubmitURL = "\(host)/form2/"

    XCTAssert(loginA1.isSignificantlyDifferentFrom(loginB))
    XCTAssert(loginA1.isSignificantlyDifferentFrom(loginC))
    XCTAssert(loginA2.isSignificantlyDifferentFrom(loginB))
    XCTAssert(loginA2.isSignificantlyDifferentFrom(loginC))
    XCTAssert(!loginA1.isSignificantlyDifferentFrom(loginA2))
  }
}
