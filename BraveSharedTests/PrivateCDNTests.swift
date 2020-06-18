// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
import BraveShared

class PrivateCDNTests: BraveSharedTests {
    
    func testMissingLength() throws {
        let data = try XCTUnwrap("ABCDEFGPPPPP".data(using: .ascii))
        XCTAssertNil(PrivateCDN.unpadded(data: data))
    }
    
    func testInvalidLength() throws {
        let length = UInt32(255)
        let raw = "ABCDEFG"
        let lengthData = withUnsafePointer(to: length) {
            Data(bytes: $0, count: 4)
        }
        let rawData = try XCTUnwrap(raw.data(using: .ascii))
        let data = lengthData + rawData
        XCTAssertNil(PrivateCDN.unpadded(data: data))
    }
    
    func testValidData() throws {
        let raw = "ABCDEFG"
        let rawData = try XCTUnwrap(raw.data(using: .ascii))
        let length = UInt32(rawData.count).bigEndian
        let lengthData = withUnsafePointer(to: length) {
            Data(bytes: $0, count: 4)
        }
        let padding = "PPPPPPPPP"
        let paddingData = try XCTUnwrap(padding.data(using: .ascii))
        
        let data = lengthData + rawData + paddingData
        let unpadded = try XCTUnwrap(PrivateCDN.unpadded(data: data))
        XCTAssertEqual(rawData, unpadded)
    }
}
