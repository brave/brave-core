// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared
import BraveShared
import Data

private let log = Logger.browserLogger

class SafeBrowsing {
    static func isSafeBrowsingEnabledForURL(_ url: URL) -> Bool {
        guard url.baseDomain != nil else {
            log.error("url: \(url) host is nil")
            return false
        }
        let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
        let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivateBrowsing)
        let isSafeBrowsingEnabled = domain.isShieldExpected(.SafeBrowsing, considerAllShieldsOption: false)
        return isSafeBrowsingEnabled
    }
}
