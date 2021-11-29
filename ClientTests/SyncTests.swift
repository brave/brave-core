// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveCore
@testable import Client

class SyncTests: XCTestCase {
    
    let validSyncCode = "9168A3FC6C2AC10EA018E7DA5AEC4D34707F3022CDBC3B209D1EE233498AC06B"
    let tooShortSyncCode = "9168A3FC6C2AC10EA018E7DA5AEC4D34707F3022CDBC3B209D1EE233498AC0"
    let tooLongSyncCode = "9168A3FC6C2AC10EA018E7DA5AEC4D34707F3022CDBC3B209D1EE233498AC06BAA"
    let validDate = Date().addingTimeInterval(BraveSyncQRCodeModel.validityDuration).timeIntervalSince1970
    let invalidDate = Date().timeIntervalSince1970
    var syncAPI: BraveSyncAPI!
    
    override func setUpWithError() throws {
        continueAfterFailure = false
        syncAPI = (UIApplication.shared.delegate as? AppDelegate)?.braveCore.syncAPI
        XCTAssertNotNil(syncAPI)
    }

    func testOldSyncVersion() throws {
        let result = BraveSyncQRCodeModel(version: BraveSyncQRCodeModel.currentlySupportedVersion - 1,
                                          syncHexCode: validSyncCode,
                                          notValidAfter: validDate).validate(syncAPI: syncAPI)
        XCTAssert(result == .insecure)
    }
    
    func testCurrentSyncVersion() throws {
        let result = BraveSyncQRCodeModel(version: BraveSyncQRCodeModel.currentlySupportedVersion,
                                          syncHexCode: validSyncCode,
                                          notValidAfter: validDate).validate(syncAPI: syncAPI)
        XCTAssert(result == .none)
    }
    
    func testNewerSyncVersion() throws {
        let result = BraveSyncQRCodeModel(version: BraveSyncQRCodeModel.currentlySupportedVersion + 1,
                                          syncHexCode: validSyncCode,
                                          notValidAfter: validDate).validate(syncAPI: syncAPI)
        XCTAssert(result == .newerVersion)
    }
    
    func testExpiredSyncDate() throws {
        let result = BraveSyncQRCodeModel(version: BraveSyncQRCodeModel.currentlySupportedVersion,
                                          syncHexCode: validSyncCode,
                                          notValidAfter: invalidDate).validate(syncAPI: syncAPI)
        XCTAssert(result == .expired)
    }
    
    func testNewerSyncDate() throws {
        let newerDate = Date().addingTimeInterval(BraveSyncQRCodeModel.futureValidityDuration).timeIntervalSince1970 + 1.minutes
        let result = BraveSyncQRCodeModel(version: BraveSyncQRCodeModel.currentlySupportedVersion,
                                          syncHexCode: validSyncCode,
                                          notValidAfter: newerDate).validate(syncAPI: syncAPI)
        XCTAssert(result == .futureDate)
    }
    
    func testInvalidSyncCode() throws {
        // Test code too short
        var result = BraveSyncQRCodeModel(version: BraveSyncQRCodeModel.currentlySupportedVersion,
                                          syncHexCode: tooShortSyncCode,
                                          notValidAfter: validDate).validate(syncAPI: syncAPI)
        XCTAssert(result == .invalidFormat)
        
        // Test code too long
        result = BraveSyncQRCodeModel(version: BraveSyncQRCodeModel.currentlySupportedVersion,
                                      syncHexCode: tooLongSyncCode,
                                      notValidAfter: validDate).validate(syncAPI: syncAPI)
        XCTAssert(result == .invalidFormat)
    }
    
    func testValidSyncCode() throws {
        var result = BraveSyncQRCodeModel(version: BraveSyncQRCodeModel.currentlySupportedVersion,
                                          syncHexCode: validSyncCode,
                                          notValidAfter: validDate).validate(syncAPI: syncAPI)
        XCTAssert(result == .none)
        
        
        let date = Date().addingTimeInterval(10.minutes).timeIntervalSince1970
        result = BraveSyncQRCodeModel(version: BraveSyncQRCodeModel.currentlySupportedVersion,
                                      syncHexCode: validSyncCode,
                                      notValidAfter: date).validate(syncAPI: syncAPI)
        XCTAssert(result == .none)
        
        result = BraveSyncQRCodeModel(syncHexCode: validSyncCode).validate(syncAPI: syncAPI)
        XCTAssert(result == .none)
    }
}
