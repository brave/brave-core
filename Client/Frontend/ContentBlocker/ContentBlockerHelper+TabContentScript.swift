/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import WebKit
import Shared
import Data
import Deferred
import BraveShared

extension ContentBlockerHelper: TabContentScript {
    class func name() -> String {
        return "TrackingProtectionStats"
    }

    func scriptMessageHandlerName() -> String? {
        return "trackingProtectionStats"
    }

    func clearPageStats() {
        stats = TPPageStats()
    }

    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        guard isEnabled,
            let body = message.body as? [String: String],
            let urlString = body["url"],
            let mainDocumentUrl = tab?.webView?.url else {
            return
        }
        
        let domain = Domain.getOrCreateForUrl(mainDocumentUrl, context: DataController.viewContext)
        if let shieldsAllOff = domain.shield_allOff, Bool(truncating: shieldsAllOff) {
            // if domain is "all_off", can just skip
            return
        }
    
        guard var components = URLComponents(string: urlString) else { return }
        components.scheme = "http"
        guard let url = components.url else { return }
        
        let resourceType = TPStatsResourceType(rawValue: body["resourceType"] ?? "")

        TPStatsBlocklistChecker.shared.isBlocked(url: url, domain: domain, resourceType: resourceType).uponQueue(.main) { listItem in
            if let listItem = listItem {
                self.stats = self.stats.create(byAddingListItem: listItem)
                
                // Increase global stats (here due to BlocklistName being in Client and BraveGlobalShieldStats being
                // in BraveShared)
                let stats = BraveGlobalShieldStats.shared
                switch listItem {
                case .ad: stats.adblock += 1
                case .https: stats.httpse += 1
                case .tracker: stats.trackingProtection += 1
                case .script: stats.scripts += 1
                case .image: stats.images += 1
                default:
                    // TODO: #97 Add fingerprinting count here when it is integrated
                    break
                }
            }
        }
    }
}
