// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Shared
import BraveShared
@testable import Client

private typealias IntroPrefs = Preferences.DefaultBrowserIntro

class DefaultBrowserIntroTests: XCTestCase {

    override func setUpWithError() throws {
        IntroPrefs.completed.reset()
        IntroPrefs.appLaunchCount.reset()
        IntroPrefs.nextShowDate.reset()
    }

    func testPrepareAndShowIfNeeded() throws {
        let now = Date()
        // First launch
        XCTAssertFalse(DefaultBrowserIntroManager.prepareAndShowIfNeeded(isNewUser: true, launchDate: now))
        
        // Second launch, same date
        XCTAssert(DefaultBrowserIntroManager.prepareAndShowIfNeeded(isNewUser: true, launchDate: now))
        
        // Third, fourth launches.. should not matter, at this point we check for date only.
        XCTAssertFalse(DefaultBrowserIntroManager.prepareAndShowIfNeeded(isNewUser: true, launchDate: now))
        XCTAssertFalse(DefaultBrowserIntroManager.prepareAndShowIfNeeded(isNewUser: true, launchDate: now))
        
        // Note: on release builds next show date is set to 5 days, on debug 5 minutes.
        let laterDate = Date(timeInterval: 2.minutes, since: now)
        XCTAssertFalse(DefaultBrowserIntroManager.prepareAndShowIfNeeded(isNewUser: true,
                                                                         launchDate: laterDate))
        
        let evenLaterDate = Date(timeInterval: 2.days, since: now)
        XCTAssert(DefaultBrowserIntroManager.prepareAndShowIfNeeded(isNewUser: true,
                                                                    launchDate: evenLaterDate))
        
        // Full new user cycle has completed
        XCTAssert(IntroPrefs.completed.value)
        
        // There should be no more callouts showing at this point
        XCTAssertFalse(DefaultBrowserIntroManager.prepareAndShowIfNeeded(isNewUser: true, launchDate: now))
        let superLateDate = Date(timeInterval: 10.days, since: now)
        XCTAssertFalse(DefaultBrowserIntroManager.prepareAndShowIfNeeded(isNewUser: true,
                                                                    launchDate: superLateDate))
    }
}
