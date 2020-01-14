/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

public struct LaunchArguments {
    public static let test = "FIREFOX_TEST"
    public static let skipIntro = "FIREFOX_SKIP_INTRO"
    public static let skipWhatsNew = "FIREFOX_SKIP_WHATS_NEW"
    public static let clearProfile = "FIREFOX_CLEAR_PROFILE"
    
    // After the colon, put the name of the file to load from test bundle
    public static let loadDatabasePrefix = "FIREFOX_LOAD_DB_NAMED:"
}
