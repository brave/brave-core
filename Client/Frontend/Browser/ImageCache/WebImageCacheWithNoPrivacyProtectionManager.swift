// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

final class WebImageCacheWithNoPrivacyProtectionManager {

    private let webImageCache: WebImageCache

    static let shared: WebImageCache = {
        let privacyProtection = NoPrivacyProtection()

        let webImageCacheManager = WebImageCacheWithNoPrivacyProtectionManager(withPrivacyProtection: privacyProtection)
        return webImageCacheManager.webImageCache
    }()

    private init(withPrivacyProtection privacyProtection: PrivacyProtectionProtocol) {
        let webImageCache = WebImageCache(withPrivacyProtection: privacyProtection)
        self.webImageCache = webImageCache
    }

}
