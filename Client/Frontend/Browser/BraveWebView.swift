// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

class BraveWebView: WKWebView {
    
    init(frame: CGRect, configuration: WKWebViewConfiguration = WKWebViewConfiguration(), privacyProtection: PrivacyProtectionProtocol = PrivacyProtection()) {
        if privacyProtection.nonPersistent {
            configuration.processPool = WKProcessPool()
            configuration.websiteDataStore = WKWebsiteDataStore.nonPersistent()
        }
        
        super.init(frame: frame, configuration: configuration)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
    
}
