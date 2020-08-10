/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import WebKit
import Shared
import Data
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
        let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
        let domain = Domain.getOrCreate(forUrl: mainDocumentUrl, persistent: !isPrivateBrowsing)
        if let shieldsAllOff = domain.shield_allOff, Bool(truncating: shieldsAllOff) {
            // if domain is "all_off", can just skip
            return
        }
    
        guard let url = URL(string: urlString) else { return }
        
        let resourceType = TPStatsResourceType(rawValue: body["resourceType"] ?? "")
        
        if resourceType == .script && domain.isShieldExpected(.NoScript, considerAllShieldsOption: true) {
            self.stats = self.stats.addingScriptBlock()
            BraveGlobalShieldStats.shared.scripts += 1
            return
        }
        
        var req = URLRequest(url: url)
        req.mainDocumentURL = mainDocumentUrl

        TPStatsBlocklistChecker.shared.isBlocked(request: req, domain: domain, resourceType: resourceType).uponQueue(.main) { listItem in
            if let listItem = listItem {
                if listItem == .https {
                    if mainDocumentUrl.scheme == "https" && url.scheme == "http" && resourceType != .image {
                        // WKWebView will block loading this URL so we can't count it due to mixed content restrictions
                        // Unfortunately, it does not check to see if a content blocker would promote said URL to https
                        // before blocking the load
                        return
                    }
                }
                self.stats = self.stats.create(byAddingListItem: listItem)
                
                // Increase global stats (here due to BlocklistName being in Client and BraveGlobalShieldStats being
                // in BraveShared)
                let stats = BraveGlobalShieldStats.shared
                switch listItem {
                case .ad: stats.adblock += 1
                case .https: stats.httpse += 1
                case .tracker: stats.trackingProtection += 1
                case .image: stats.images += 1
                case .https: stats.httpse += 1
                default:
                    // TODO: #97 Add fingerprinting count here when it is integrated
                    break
                }
            }
        }
    }
}
