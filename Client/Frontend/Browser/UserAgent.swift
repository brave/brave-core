// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

struct UserAgent {
    static let mobile = UserAgentBuilder().build(desktopMode: false)
    static let desktop = UserAgentBuilder().build(desktopMode: true)
    
    static var userAgentForDesktopMode: String {
        UserAgent.shouldUseDesktopMode ? UserAgent.desktop : UserAgent.mobile
    }
    
    static var shouldUseDesktopMode: Bool {
        if UIDevice.isIpad {
            return Preferences.General.alwaysRequestDesktopSite.value
        }
        
        return false
    }
}
