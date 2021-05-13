/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
@testable import Storage
import XCGLogger

import XCTest

private let log = XCGLogger.default

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
        self.removeAllLogins().upon({ res in expectation.fulfill() })
        waitForExpectations(timeout: 10.0, handler: nil)
    }

    func testAddLogin() {
        log.debug("Created \(self.login)")
        let expectation = self.expectation(description: "Add login")

        addLogin(login)
            >>> getLoginsFor(login.protectionSpace, expected: [login])
            >>> done(expectation)

        waitForExpectations(timeout: 10.0, handler: nil)
    }

    func testGetOrder() {
        let expectation = self.expectation(description: "Add login")

        // Different GUID.
        let login2 = Login.createWithHostname("hostname1", username: "username2", password: "password2")
        login2.formSubmitURL = "http://submit.me"

        addLogin(login) >>> { self.addLogin(login2) } >>>
            getLoginsFor(login.protectionSpace, expected: [login2, login]) >>>
            done(expectation)

        waitForExpectations(timeout: 10.0, handler: nil)
    }

    func testRemoveLogin() {
        let expectation = self.expectation(description: "Remove login")

        addLogin(login)
            >>> { self.removeLogin(self.login) }
            >>> getLoginsFor(login.protectionSpace, expected: [])
            >>> done(expectation)

        waitForExpectations(timeout: 10.0, handler: nil)
    }

    func testRemoveLogins() {
        let loginA = Login.createWithHostname("alphabet.com", username: "username1", password: "password1", formSubmitURL: formSubmitURL)
        let loginB = Login.createWithHostname("alpha.com", username: "username2", password: "password2", formSubmitURL: formSubmitURL)
        let loginC = Login.createWithHostname("berry.com", username: "username3", password: "password3", formSubmitURL: formSubmitURL)
        let loginD = Login.createWithHostname("candle.com", username: "username4", password: "password4", formSubmitURL: formSubmitURL)

        func addLogins() -> Success {
            addLogin(loginA).succeeded()
            addLogin(loginB).succeeded()
            addLogin(loginC).succeeded()
            addLogin(loginD).succeeded()
            return succeed()
        }

        addLogins().succeeded()
        let guids = [loginA.guid, loginB.guid]
        logins.removeLoginsWithGUIDs(guids).succeeded()
        let result = logins.getAllLogins().value.successValue!
        XCTAssertEqual(result.count, 2)
    }
    
    func testRemoveManyLogins() {
        log.debug("Remove a large number of logins at once")
        var guids: [GUID] = []
        for i in 0..<2000 {
            let login = Login.createWithHostname("mozilla.org", username: "Fire", password: "fox", formSubmitURL: formSubmitURL)
            if i <= 1000 {
                guids += [login.guid]
            }
            addLogin(login).succeeded()
        }
        logins.removeLoginsWithGUIDs(guids).succeeded()
        let result = logins.getAllLogins().value.successValue!
        XCTAssertEqual(result.count, 999)
    }

    func testUpdateLogin() {
        let expectation = self.expectation(description: "Update login")
        let updated = Login.createWithHostname("hostname1", username: "username1", password: "password3", formSubmitURL: formSubmitURL)
        updated.guid = self.login.guid

        addLogin(login) >>> { self.updateLogin(updated) } >>>
            getLoginsFor(login.protectionSpace, expected: [updated]) >>>
            done(expectation)

        waitForExpectations(timeout: 10.0, handler: nil)
    }

    func testAddInvalidLogin() {
        let emptyPasswordLogin = Login.createWithHostname("hostname1", username: "username1", password: "", formSubmitURL: formSubmitURL)
        var result =  logins.addLogin(emptyPasswordLogin).value
        XCTAssertNil(result.successValue)
        XCTAssertNotNil(result.failureValue)
        XCTAssertEqual(result.failureValue?.description, "Can't add a login with an empty password.")

        let emptyHostnameLogin = Login.createWithHostname("", username: "username1", password: "password", formSubmitURL: formSubmitURL)
        result =  logins.addLogin(emptyHostnameLogin).value
        XCTAssertNil(result.successValue)
        XCTAssertNotNil(result.failureValue)
        XCTAssertEqual(result.failureValue?.description, "Can't add a login with an empty hostname.")

        let credential = URLCredential(user: "username", password: "password", persistence: .forSession)
        let protectionSpace = URLProtectionSpace(host: "https://website.com", port: 443, protocol: "https", realm: "Basic Auth", authenticationMethod: "Basic Auth")
        let bothFormSubmitURLAndRealm = Login.createWithCredential(credential, protectionSpace: protectionSpace)
        bothFormSubmitURLAndRealm.formSubmitURL = "http://submit.me"
        result =  logins.addLogin(bothFormSubmitURLAndRealm).value
        XCTAssertNil(result.successValue)
        XCTAssertNotNil(result.failureValue)
        XCTAssertEqual(result.failureValue?.description, "Can't add a login with both a httpRealm and formSubmitURL.")

        let noFormSubmitURLOrRealm = Login.createWithHostname("host", username: "username1", password: "password", formSubmitURL: nil)
        result =  logins.addLogin(noFormSubmitURLOrRealm).value
        XCTAssertNil(result.successValue)
        XCTAssertNotNil(result.failureValue)
        XCTAssertEqual(result.failureValue?.description, "Can't add a login without a httpRealm or formSubmitURL.")
    }

    func testUpdateInvalidLogin() {
        let updated = Login.createWithHostname("hostname1", username: "username1", password: "", formSubmitURL: formSubmitURL)
        updated.guid = self.login.guid

        addLogin(login).succeeded()
        var result = logins.updateLoginByGUID(login.guid, new: updated, significant: true).value
        XCTAssertNil(result.successValue)
        XCTAssertNotNil(result.failureValue)
        XCTAssertEqual(result.failureValue?.description, "Can't add a login with an empty password.")

        let emptyHostnameLogin = Login.createWithHostname("", username: "username1", password: "", formSubmitURL: formSubmitURL)
        emptyHostnameLogin.guid = self.login.guid
        result = logins.updateLoginByGUID(login.guid, new: emptyHostnameLogin, significant: true).value
        XCTAssertNil(result.successValue)
        XCTAssertNotNil(result.failureValue)
        XCTAssertEqual(result.failureValue?.description, "Can't add a login with an empty hostname.")

        let credential = URLCredential(user: "username", password: "password", persistence: .forSession)
        let protectionSpace = URLProtectionSpace(host: "https://website.com", port: 443, protocol: "https", realm: "Basic Auth", authenticationMethod: "Basic Auth")
        let bothFormSubmitURLAndRealm = Login.createWithCredential(credential, protectionSpace: protectionSpace)
        bothFormSubmitURLAndRealm.formSubmitURL = "http://submit.me"
        bothFormSubmitURLAndRealm.guid = self.login.guid
        result = logins.updateLoginByGUID(login.guid, new: bothFormSubmitURLAndRealm, significant: true).value
        XCTAssertNil(result.successValue)
        XCTAssertNotNil(result.failureValue)
        XCTAssertEqual(result.failureValue?.description, "Can't add a login with both a httpRealm and formSubmitURL.")

        let noFormSubmitURLOrRealm = Login.createWithHostname("host", username: "username1", password: "password", formSubmitURL: nil)
        noFormSubmitURLOrRealm.guid = self.login.guid
        result = logins.updateLoginByGUID(login.guid, new: noFormSubmitURLOrRealm, significant: true).value
        XCTAssertNil(result.successValue)
        XCTAssertNotNil(result.failureValue)
        XCTAssertEqual(result.failureValue?.description, "Can't add a login without a httpRealm or formSubmitURL.")
    }

    func testSearchLogins() {
        let loginA = Login.createWithHostname("alphabet.com", username: "username1", password: "password1", formSubmitURL: formSubmitURL)
        let loginB = Login.createWithHostname("alpha.com", username: "username2", password: "password2", formSubmitURL: formSubmitURL)
        let loginC = Login.createWithHostname("berry.com", username: "username3", password: "password3", formSubmitURL: formSubmitURL)
        let loginD = Login.createWithHostname("candle.com", username: "username4", password: "password4", formSubmitURL: formSubmitURL)

        func addLogins() -> Success {
            addLogin(loginA).succeeded()
            addLogin(loginB).succeeded()
            addLogin(loginC).succeeded()
            addLogin(loginD).succeeded()
            return succeed()
        }

        func checkAllLogins() -> Success {
            return logins.getAllLogins() >>== { results in
                XCTAssertEqual(results.count, 4)
                return succeed()
            }
        }

        func checkSearchHostnames() -> Success {
            return logins.searchLoginsWithQuery("pha") >>== { results in
                XCTAssertEqual(results.count, 2)
                XCTAssertEqual(results[0]!.hostname, "http://alpha.com")
                XCTAssertEqual(results[1]!.hostname, "http://alphabet.com")
                return succeed()
            }
        }

        func checkSearchUsernames() -> Success {
            return logins.searchLoginsWithQuery("username") >>== { results in
                XCTAssertEqual(results.count, 4)
                XCTAssertEqual(results[0]!.username, "username2")
                XCTAssertEqual(results[1]!.username, "username1")
                XCTAssertEqual(results[2]!.username, "username3")
                XCTAssertEqual(results[3]!.username, "username4")
                return succeed()
            }
        }

        func checkSearchPasswords() -> Success {
            return logins.searchLoginsWithQuery("pass") >>== { results in
                XCTAssertEqual(results.count, 4)
                XCTAssertEqual(results[0]!.password, "password2")
                XCTAssertEqual(results[1]!.password, "password1")
                XCTAssertEqual(results[2]!.password, "password3")
                XCTAssertEqual(results[3]!.password, "password4")
                return succeed()
            }
        }

        XCTAssertTrue(addLogins().value.isSuccess)

        XCTAssertTrue(checkAllLogins().value.isSuccess)
        XCTAssertTrue(checkSearchHostnames().value.isSuccess)
        XCTAssertTrue(checkSearchUsernames().value.isSuccess)
        XCTAssertTrue(checkSearchPasswords().value.isSuccess)

        XCTAssertTrue(removeAllLogins().value.isSuccess)
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

    func done(_ expectation: XCTestExpectation) -> () -> Success {
        return {
            self.removeAllLogins()
               >>> self.getLoginsFor(self.login.protectionSpace, expected: [])
               >>> {
                    expectation.fulfill()
                    return succeed()
                }
        }
    }

    // Note: These functions are all curried so that we pass arguments, but still chain them below
    func addLogin(_ login: LoginData) -> Success {
        log.debug("Add \(login)")
        return logins.addLogin(login)
    }

    func updateLogin(_ login: LoginData) -> Success {
        log.debug("Update \(login)")
        return logins.updateLoginByGUID(login.guid, new: login, significant: true)
    }

    func addUseDelayed(_ login: Login, time: UInt32) -> Success {
        sleep(time)
        login.timeLastUsed = Date.nowMicroseconds()
        let res = logins.addUseOfLoginByGUID(login.guid)
        sleep(time)
        return res
    }

    func getLoginsFor(_ protectionSpace: URLProtectionSpace, expected: [LoginData]) -> (() -> Success) {
        return {
            log.debug("Get logins for \(protectionSpace)")
            return self.logins.getLoginsForProtectionSpace(protectionSpace) >>== { results in
                XCTAssertEqual(expected.count, results.count)
                for (index, login) in expected.enumerated() {
                    XCTAssertEqual(results[index]!.username!, login.username!)
                    XCTAssertEqual(results[index]!.hostname, login.hostname)
                    XCTAssertEqual(results[index]!.password, login.password)
                }
                return succeed()
            }
        }
    }

    /*
    func getLoginDetailsFor(login: LoginData, expected: LoginUsageData) -> (() -> Success) {
        return {
            log.debug("Get details for \(login)")
            let deferred = self.logins.getUsageDataForLogin(login)
            log.debug("Final result \(deferred)")
            return deferred >>== { l in
                log.debug("Got cursor")
                XCTAssertLessThan(expected.timePasswordChanged - l.timePasswordChanged, 10)
                XCTAssertLessThan(expected.timeLastUsed - l.timeLastUsed, 10)
                XCTAssertLessThan(expected.timeCreated - l.timeCreated, 10)
                return succeed()
            }
        }
    }
    */

    func removeLogin(_ login: LoginData) -> Success {
        log.debug("Remove \(login)")
        return logins.removeLoginByGUID(login.guid)
    }

    func removeAllLogins() -> Success {
        log.debug("Remove All")
        // Because we don't want to just mark them as deleted.
        return self.db.run("DELETE FROM loginsM") >>> { self.db.run("DELETE FROM loginsL") }
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

    func testLoginsSearchMatchOnePerf() {
        populateTestLogins()

        // Measure time to find one entry amongst the 1000 of them
        self.measureMetrics([XCTPerformanceMetric.wallClockTime], automaticallyStartMeasuring: true) {
            for _ in 0...5 {
                self.logins.searchLoginsWithQuery("username500").succeeded()
            }
            self.stopMeasuring()
        }

        XCTAssertTrue(removeAllLogins().value.isSuccess)
    }

    func testLoginsSearchMatchAllPerf() {
        populateTestLogins()

        // Measure time to find all matching results
        self.measureMetrics([XCTPerformanceMetric.wallClockTime], automaticallyStartMeasuring: true) {
            for _ in 0...5 {
                self.logins.searchLoginsWithQuery("username").succeeded()
            }
            self.stopMeasuring()
        }

        XCTAssertTrue(removeAllLogins().value.isSuccess)
    }

    func testLoginsGetAllPerf() {
        populateTestLogins()

        // Measure time to find all matching results
        self.measureMetrics([XCTPerformanceMetric.wallClockTime], automaticallyStartMeasuring: true) {
            for _ in 0...5 {
                self.logins.getAllLogins().succeeded()
            }
            self.stopMeasuring()
        }

        XCTAssertTrue(removeAllLogins().value.isSuccess)
    }

    func populateTestLogins() {
        for i in 0..<1000 {
            let login = Login.createWithHostname("website\(i).com", username: "username\(i)", password: "password\(i)", formSubmitURL: "test")
            addLogin(login).succeeded()
        }
    }

    func addLogin(_ login: LoginData) -> Success {
        return logins.addLogin(login)
    }

    func removeAllLogins() -> Success {
        log.debug("Remove All")
        // Because we don't want to just mark them as deleted.
        return self.db.run("DELETE FROM loginsM") >>> { self.db.run("DELETE FROM loginsL") }
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
        self.removeAllLogins().upon({ res in expectation.fulfill() })
        waitForExpectations(timeout: 10.0, handler: nil)
    }

    func removeAllLogins() -> Success {
        log.debug("Remove All")
        // Because we don't want to just mark them as deleted.
        return self.db.run("DELETE FROM loginsM") >>> { self.db.run("DELETE FROM loginsL") }
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
