// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Client
import CoreMedia

class PlaylistTests: XCTestCase {
    func testVideoPlayerTrackBarTimeFormatter() throws {
        XCTAssert(VideoTrackerBar.timeToString(.zero) == "00:00") //mm:ss
        XCTAssert(VideoTrackerBar.timeToString(CMTimeMakeWithSeconds(1.0, preferredTimescale: 1)) == "00:01") //mm:ss
        XCTAssert(VideoTrackerBar.timeToString(CMTimeMakeWithSeconds(30.0, preferredTimescale: 1)) == "00:30") //mm:ss
        XCTAssert(VideoTrackerBar.timeToString(CMTimeMakeWithSeconds(60.0, preferredTimescale: 1)) == "01:00") //mm:ss
        XCTAssert(VideoTrackerBar.timeToString(CMTimeMakeWithSeconds(60 * 60, preferredTimescale: 1)) == "01:00:00") //hh:mm:ss
        XCTAssert(VideoTrackerBar.timeToString(CMTimeMakeWithSeconds(60 * 60 * 24, preferredTimescale: 1)) == "1d 00:00:00") //dd hh:mm:ss
    }
    
    func testVideoPlayerTrackBarTimeManualFormatter() throws {
        XCTAssert(VideoTrackerBar.timeToString(.zero) == "00:00") //mm:ss
        XCTAssert(VideoTrackerBar.timeToString(CMTimeMakeWithSeconds(1.0, preferredTimescale: 1)) == "00:01") //mm:ss
        XCTAssert(VideoTrackerBar.timeToString(CMTimeMakeWithSeconds(30.0, preferredTimescale: 1)) == "00:30") //mm:ss
        XCTAssert(VideoTrackerBar.timeToString(CMTimeMakeWithSeconds(60.0, preferredTimescale: 1)) == "01:00") //mm:ss
        XCTAssert(VideoTrackerBar.timeToString(CMTimeMakeWithSeconds(60 * 60, preferredTimescale: 1)) == "01:00:00") //hh:mm:ss
        XCTAssert(VideoTrackerBar.timeToString(CMTimeMakeWithSeconds(60 * 60 * 24, preferredTimescale: 1)) == "01:00:00:00") //dd:hh:mm:ss
    }
}
