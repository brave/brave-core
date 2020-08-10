// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import WebKit
import Shared
import Data
import BraveShared

private let log = Logger.browserLogger

class ContentBlockerRegion: BlocklistName {
    /// Static content blocker stores rule lists for regional ad blocking.
    private static var regionalContentBlocker: ContentBlockerRegion?
    
    private let localeCode: String
    
    private init(localeCode: String, filename: String) {
        self.localeCode = localeCode
        super.init(filename: filename)
    }
}

extension ContentBlockerRegion {

    /// Get a `ContentBlockerRegion` for a given locale if one exists for that region
    static func with(localeCode code: String?) -> ContentBlockerRegion? {
        guard let code = code else {
            log.error("No locale string was provided")
            return nil
        }

        // Check if regional resource exists for a given locale.
        if ResourceLocale(rawValue: code) == nil { return nil }
        
        // This handles two cases, regional content blocker is nil,
        // or locale has changed and the content blocker needs to be reassigned
        if regionalContentBlocker?.localeCode != code {
            regionalContentBlocker = ContentBlockerRegion(localeCode: code, filename: code)
        }
        
        return regionalContentBlocker
    }
}
