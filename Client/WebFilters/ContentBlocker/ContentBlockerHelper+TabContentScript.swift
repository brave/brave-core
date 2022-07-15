/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import WebKit
import Shared
import Data
import BraveShared
import BraveCore

private let log = Logger.braveCoreLogger

extension ContentBlockerHelper: TabContentScript {
  private struct ContentBlockerDTO: Decodable {
    struct ContentblockerDTOData: Decodable {
      let resourceType: AdblockEngine.ResourceType
      let resourceURL: String
      let sourceURL: String
    }
    
    let securityToken: String
    let data: ContentblockerDTOData
  }
  
  class func name() -> String {
    return "TrackingProtectionStats"
  }

  func scriptMessageHandlerName() -> String? {
    return "trackingProtectionStats"
  }

  func clearPageStats() {
    stats = TPPageStats()
    blockedRequests.removeAll()
  }

  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: (Any?, String?) -> Void) {
    defer { replyHandler(nil, nil) }
    guard isEnabled else { return }
    
    guard let currentTabURL = tab?.webView?.url else {
      assertionFailure("Missing tab or webView")
      return
    }
    
    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(ContentBlockerDTO.self, from: data)
      
      guard dto.securityToken ==  UserScriptManager.messageHandlerTokenString else {
        assertionFailure("Invalid security token.")
        return
      }
    
      let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
      let domain = Domain.getOrCreate(forUrl: currentTabURL, persistent: !isPrivateBrowsing)
      if let shieldsAllOff = domain.shield_allOff, Bool(truncating: shieldsAllOff) {
        // if domain is "all_off", can just skip
        return
      }

      if dto.data.resourceType == .script && domain.isShieldExpected(.NoScript, considerAllShieldsOption: true) {
        self.stats = self.stats.adding(scriptCount: 1)
        BraveGlobalShieldStats.shared.scripts += 1
        return
      }
      
      // Because javascript urls allow some characters that `URL` does not,
      // we use `NSURL(idnString: String)` to parse them
      guard let requestURL = NSURL(idnString: dto.data.resourceURL) as URL? else { return }
      guard let sourceURL = NSURL(idnString: dto.data.sourceURL) as URL? else { return }
      guard let domainURLString = domain.url else { return }
      
      // Getting this domain and current tab urls before going into asynchronous closure
      // to avoid threading problems(#1094, #1096)
      assertIsMainThread("Getting enabled blocklists should happen on main thread")
      let enabledLists = BlocklistName.blocklists(forDomain: domain).on

      TPStatsBlocklistChecker.shared.isBlocked(
        requestURL: requestURL,
        sourceURL: sourceURL,
        enabledLists: enabledLists,
        resourceType: dto.data.resourceType
      ) { listItem in
        guard let listItem = listItem else { return }
 
        if listItem == .https && dto.data.resourceType != .image && currentTabURL.scheme == "https" && requestURL.scheme == "http" {
          // WKWebView will block loading this URL so we can't count it due to mixed content restrictions
          // Unfortunately, it does not check to see if a content blocker would promote said URL to https
          // before blocking the load
          return
        }
        
        assertIsMainThread("Result should happen on the main thread")
        
        if listItem == .ad, Preferences.PrivacyReports.captureShieldsData.value,
           let domainURL = URL(string: domainURLString),
           let blockedResourceHost = requestURL.baseDomain,
           !PrivateBrowsingManager.shared.isPrivateBrowsing {
          PrivacyReportsManager.pendingBlockedRequests.append((blockedResourceHost, domainURL, Date()))
        }
        
        // First check to make sure we're not counting the same repetitive requests multiple times
        guard !self.blockedRequests.contains(requestURL) else { return }
        self.blockedRequests.insert(requestURL)

        // Increase global stats (here due to BlocklistName being in Client and BraveGlobalShieldStats being
        // in BraveShared)
        let stats = BraveGlobalShieldStats.shared
        switch listItem {
        case .ad:
          stats.adblock += 1
          self.stats = self.stats.adding(adCount: 1)
        case .https:
          stats.httpse += 1
          self.stats = self.stats.adding(httpsCount: 1)
        case .tracker:
          stats.trackingProtection += 1
          self.stats = self.stats.adding(trackerCount: 1)
        case .image:
          stats.images += 1
        default:
          // TODO: #97 Add fingerprinting count here when it is integrated
          break
        }
      }
    } catch {
      log.error(error)
    }
  }
}
