// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveShared
import Shared

class DAUTests: XCTestCase {
    
    // 7-7-07 at 12noon GMT
    private let dau = DAU(date: Date(timeIntervalSince1970: 1183809600))
    
    func testChannelParam() {
        let releaseExpected = URLQueryItem(name: "channel", value: "stable")
        XCTAssertEqual(dau.channelParam(for: .release), releaseExpected)
        
        let betaExpected = URLQueryItem(name: "channel", value: "beta")
        XCTAssertEqual(dau.channelParam(for: .beta), betaExpected)
        
        let devExpected = URLQueryItem(name: "channel", value: "beta")
        XCTAssertEqual(dau.channelParam(for: .developer), devExpected)
    }
    
    func testVersionParam() {
        var expected = URLQueryItem(name: "version", value: "1.1.0")
        XCTAssertEqual(dau.versionParam(for: "1.1"), expected)
        
        expected = URLQueryItem(name: "version", value: "1.1.1")
        XCTAssertEqual(dau.versionParam(for: "1.1.1"), expected)
    }
    
    func testShouldAppend0() {
        XCTAssertTrue(DAU.shouldAppend0(toVersion: "1.1"))
        XCTAssertFalse(DAU.shouldAppend0(toVersion: "1.1.1"))
    }
    
    func testFirstLaunchParam() {
        var expected: URLQueryItem!
        expected = URLQueryItem(name: "first", value: "true")
        XCTAssertEqual(dau.firstLaunchParam(for: true), expected)
        
        expected = URLQueryItem(name: "first", value: "false")
        XCTAssertEqual(dau.firstLaunchParam(for: false), expected)
    }
    
    func testWOIParam() {
        var expected: URLQueryItem!
        expected = URLQueryItem(name: "woi", value: DAU.defaultWoiDate)
        XCTAssertEqual(dau.weekOfInstallationParam(for: nil), expected)
        
        expected = URLQueryItem(name: "woi", value: "absolutelyNothing")
        XCTAssertEqual(dau.weekOfInstallationParam(for: "absolutelyNothing"), expected)
        
        expected = URLQueryItem(name: "woi", value: "2012-12-12")
        XCTAssertEqual(dau.weekOfInstallationParam(for: "2012-12-12"), expected)
    }
    
    func testStatParamsInvalidInputs() {
        XCTAssertNil(dau.dauStatParams(nil, firstPing: false, channel: .beta))
        XCTAssertNil(dau.dauStatParams(nil, firstPing: false, channel: .release))
        XCTAssertNil(dau.dauStatParams([], firstPing: false, channel: .beta))
        XCTAssertNil(dau.dauStatParams([1,2], firstPing: false, channel: .beta))
        XCTAssertNil(dau.dauStatParams([1,2,3,4], firstPing: false, channel: .beta))
    }
    
    func testStatParamsValidInputs() {
        var expected: [URLQueryItem]!
        
        func daily(_ v: Bool) -> URLQueryItem { return .init(name: "daily", value: v.description) }
        func weekly(_ v: Bool) -> URLQueryItem { return .init(name: "weekly", value: v.description) }
        func monthly(_ v: Bool) -> URLQueryItem { return .init(name: "monthly", value: v.description) }
        
        expected = [ daily(true), weekly(true), monthly(true) ]
        
        func functionallyEquivalent(_ first: [URLQueryItem], _ second: [URLQueryItem]) -> Bool {
            if first.count != second.count { return false }
            for item in first {
                if !second.contains(item) { return false }
            }
            return true
        }
        XCTAssertTrue(functionallyEquivalent(dau.dauStatParams(firstPing: true)!, expected))
        XCTAssertTrue(functionallyEquivalent(dau.dauStatParams(firstPing: false, channel: .developer)!, expected))
    }
}
