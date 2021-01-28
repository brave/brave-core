// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

private typealias IntroPrefs = Preferences.DefaultBrowserIntro

struct DefaultBrowserIntroManager {
    /// This function should be called when the app is initialized.
    /// It determines whether we should show default browser intro popup or not and sets corresponding preferences accordingly.
    /// Returns true if the popup should be shown.
    @discardableResult
    static func prepareAndShowIfNeeded(isNewUser: Bool, launchDate: Date = Date()) -> Bool {
        if IntroPrefs.completed.value { return false }
        
        IntroPrefs.appLaunchCount.value += 1
        
        if IntroPrefs.appLaunchCount.value == 2 {
            let nextDateToShow = AppConstants.buildChannel.isPublic ? 5.days : 5.minutes
            
            IntroPrefs.nextShowDate.value =
                Date(timeIntervalSinceNow: nextDateToShow)
            return true
        }
        
        if let nextShowDate = IntroPrefs.nextShowDate.value,
           launchDate > nextShowDate {
            IntroPrefs.completed.value = true
            IntroPrefs.nextShowDate.value = nil
            return true
        }
        
        return false
    }
}
