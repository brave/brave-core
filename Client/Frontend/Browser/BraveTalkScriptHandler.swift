// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared
import BraveShared
import BraveCore

private let log = Logger.browserLogger

class BraveTalkScriptHandler: TabContentScript {
    private weak var tab: Tab?
    private weak var rewards: BraveRewards?
        
    required init(tab: Tab, rewards: BraveRewards) {
        self.tab = tab
        self.rewards = rewards
        
        tab.rewardsEnabledCallback = { [weak self] success in
            self?.callback(result: success)
        }
    }
    
    static func name() -> String { "BraveTalkHelper" }
    
    func scriptMessageHandlerName() -> String? { BraveTalkScriptHandler.name() }
    
    func userContentController(_ userContentController: WKUserContentController,
                               didReceiveScriptMessage message: WKScriptMessage) {
        let allowedHosts = DomainUserScript.braveTalk.associatedDomains
        
        guard let requestHost = message.frameInfo.request.url?.host,
              allowedHosts.contains(requestHost),
              message.frameInfo.isMainFrame else {
            log.error("Backup search request called from disallowed host")
            return
        }
        
        handleBraveRequestAdsEnabled()
    }
    
    private func handleBraveRequestAdsEnabled() {
        
        guard let rewards = rewards, !PrivateBrowsingManager.shared.isPrivateBrowsing else {
            callback(result: false)
            return
        }
        
        if rewards.isEnabled {
            callback(result: true)
            return
        }
        
        // If rewards are disabled we show a Rewards panel,
        // The `callback` will be called from other place.
        if let tab = tab {
            tab.tabDelegate?.showRequestRewardsPanel(tab)
        }
    }
    
    private func callback(result: Bool) {
        let functionName =
            "window.__firefox__.BT\(UserScriptManager.messageHandlerTokenString).resolve"
        
        // Have to use old evaluateJavaScript to escape bool value properly.
        // TODO: Switch to proper implementation once #3824 is done.
        // swiftlint:disable:next safe_javascript
        tab?.webView?.evaluateJavaScript(functionName + "(1, \(result))") { _, error  in
            if let error = error {
                log.error("BraveTalk api error: \(error)")
            }
        }
    }
}
