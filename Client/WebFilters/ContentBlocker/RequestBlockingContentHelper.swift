// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit
import BraveCore
import BraveShared
import Data

private let log = Logger.braveCoreLogger

class RequestBlockingContentHelper: TabContentScript {
  private struct RequestBlockingDTO: Decodable {
    struct RequestBlockingDTOData: Decodable, Hashable {
      let resourceType: AdblockEngine.ResourceType
      let resourceURL: String
      let sourceURL: String
    }
    
    let securityToken: String
    let data: RequestBlockingDTOData
  }
  
  static func name() -> String {
    return "RequestBlockingContentHelper"
  }
  
  static func scriptMessageHandlerName() -> String {
    return ["requestBlockingContentHelper", UserScriptManager.messageHandlerTokenString].joined(separator: "_")
  }
  
  private weak var tab: Tab?
  private var blockedRequests: Set<URL> = []
  
  init(tab: Tab) {
    self.tab = tab
  }
  
  func clearBlockedRequests() {
    blockedRequests.removeAll()
  }
  
  func scriptMessageHandlerName() -> String? {
    return Self.scriptMessageHandlerName()
  }
  
  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: @escaping (Any?, String?) -> Void) {
    guard let tab = tab, let currentTabURL = tab.webView?.url else {
      assertionFailure("Should have a tab set")
      return
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(RequestBlockingDTO.self, from: data)
      
      guard dto.securityToken == UserScriptManager.securityTokenString else {
        assertionFailure("Invalid security token. Fix the `RequestBlocking.js` script")
        replyHandler(false, nil)
        return
      }
      
      // Because javascript urls allow some characters that `URL` does not,
      // we use `NSURL(idnString: String)` to parse them
      guard let requestURL = NSURL(idnString: dto.data.resourceURL) as URL? else { return }
      guard let sourceURL = NSURL(idnString: dto.data.sourceURL) as URL? else { return }
      let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
      let domain = Domain.getOrCreate(forUrl: currentTabURL, persistent: !isPrivateBrowsing)
      guard let domainURLString = domain.url else { return }
      
      AdBlockStats.shared.shouldBlock(
        requestURL: requestURL,
        sourceURL: sourceURL,
        resourceType: dto.data.resourceType
      ) { [weak self] shouldBlock in
        assertIsMainThread("Result should happen on the main thread")
        
        if shouldBlock, Preferences.PrivacyReports.captureShieldsData.value,
           let domainURL = URL(string: domainURLString),
           let blockedResourceHost = requestURL.baseDomain,
           !PrivateBrowsingManager.shared.isPrivateBrowsing {
          PrivacyReportsManager.pendingBlockedRequests.append((blockedResourceHost, domainURL, Date()))
        }

        if shouldBlock && !(self?.blockedRequests.contains(requestURL) ?? true) {
          BraveGlobalShieldStats.shared.adblock += 1
          let stats = tab.contentBlocker.stats
          tab.contentBlocker.stats = stats.adding(adCount: 1)
          self?.blockedRequests.insert(requestURL)
        }
        
        replyHandler(shouldBlock, nil)
      }
    } catch {
      assertionFailure("Invalid type of message. Fix the `RequestBlocking.js` script")
      replyHandler(false, nil)
    }
  }
}
