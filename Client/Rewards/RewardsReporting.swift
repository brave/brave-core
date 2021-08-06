// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import BraveCore
import XCGLogger
import Shared

private let log = Logger.braveCoreLogger

class RewardsReporting: TabContentScript {
    let rewards: BraveRewards
    weak var tab: Tab?
    
    init(rewards: BraveRewards, tab: Tab) {
        self.rewards = rewards
        self.tab = tab
    }
    
    class func name() -> String {
        return "RewardsReporting"
    }
    
    func scriptMessageHandlerName() -> String? {
        return "rewardsReporting"
    }
    
    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        struct Content: Decodable {
            var method: String
            var url: String
            var data: String?
            var referrerUrl: String?
        }
        
        if PrivateBrowsingManager.shared.isPrivateBrowsing || !rewards.isEnabled {
            return
        }
        
        do {
            guard let body = message.body as? [String: AnyObject] else {
                return
            }
            
            if UserScriptManager.isMessageHandlerTokenMissing(in: body) {
                log.debug("Missing required security token.")
                return
            }
            
            if let body = body["data"] as? [String: AnyObject] {
                let json = try JSONSerialization.data(withJSONObject: body, options: [])
                var content = try JSONDecoder().decode(Content.self, from: json)
                
                guard let tab = tab, let tabURL = tab.url else { return }
                
                if content.url.hasPrefix("//") {
                    content.url = "\(tabURL.scheme ?? "http"):\(content.url)"
                }
                
                guard let url = URL(string: content.url) else { return }
                let refURL = URL(string: content.referrerUrl ?? "")
                
                if content.method.lowercased() == "post" {
                    if let postData = content.data?.removingPercentEncoding, let data = postData.data(using: .utf8) {
                        if BraveLedger.isMediaURL(url, firstPartyURL: tabURL, referrerURL: refURL) {
                            rewards.reportPostData(data, url: url, tabId: Int(tab.rewardsId), firstPartyURL: tabURL, referrerURL: refURL)
                        }
                    }
                } else {
                    rewards.reportXHRLoad(url: url, tabId: Int(tab.rewardsId), firstPartyURL: tabURL, referrerURL: refURL)
                }
            }
        } catch {
            log.error("Failed to parse message from rewards reporting JS: \(error)")
        }
    }
}
