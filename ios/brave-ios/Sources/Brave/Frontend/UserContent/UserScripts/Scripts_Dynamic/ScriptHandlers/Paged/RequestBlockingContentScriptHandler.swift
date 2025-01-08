// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Data
import Foundation
import Preferences
import Shared
import WebKit

class RequestBlockingContentScriptHandler: TabContentScript {
  struct RequestBlockingDTO: Decodable {
    struct RequestBlockingDTOData: Decodable, Hashable {
      let resourceType: AdblockEngine.ResourceType
      let resourceURL: String
      let windowOrigin: String
    }

    let securityToken: String
    let data: RequestBlockingDTOData
  }

  static let scriptName = "RequestBlockingScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }

    return WKUserScript(
      source: secureScript(
        handlerName: messageHandlerName,
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    guard let currentTabURL = tab.webView?.url else {
      assertionFailure("Should have a tab set")
      return
    }

    if !verifyMessage(message: message) {
      assertionFailure("Invalid security token. Fix the `RequestBlocking.js` script")
      replyHandler(false, nil)
      return
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(RequestBlockingDTO.self, from: data)

      // Because javascript urls allow some characters that `URL` does not,
      // we use `NSURL(idnString: String)` to parse them
      guard let requestURL = NSURL(idnString: dto.data.resourceURL) as URL? else { return }
      guard let windowOriginURL = NSURL(idnString: dto.data.windowOrigin) as URL? else { return }
      let isPrivateBrowsing = tab.isPrivate

      Task { @MainActor in
        let domain = Domain.getOrCreate(forUrl: currentTabURL, persistent: !isPrivateBrowsing)
        guard let domainURLString = domain.url else { return }
        let shouldBlock = await AdBlockGroupsManager.shared.shouldBlock(
          requestURL: requestURL,
          sourceURL: windowOriginURL,
          resourceType: dto.data.resourceType,
          domain: domain
        )

        // Ensure we check that the stats we're tracking is still for the same page
        // Some web pages (like youtube) like to rewrite their main frame urls
        // so we check the source etld+1 agains the tab url etld+1
        // For subframes which may use different etld+1 than the main frame (example `reddit.com` and `redditmedia.com`)
        // We simply check the known subframeURLs on this page.
        guard
          tab.url?.baseDomain == windowOriginURL.baseDomain
            || tab.currentPageData?.allSubframeURLs.contains(windowOriginURL) == true
        else {
          replyHandler(shouldBlock, nil)
          return
        }

        if shouldBlock, Preferences.PrivacyReports.captureShieldsData.value,
          let domainURL = URL(string: domainURLString),
          let blockedResourceHost = requestURL.baseDomain,
          !isPrivateBrowsing
        {
          PrivacyReportsManager.pendingBlockedRequests.append(
            (blockedResourceHost, domainURL, Date())
          )
        }

        if shouldBlock
          && !tab.contentBlocker.blockedRequests.contains(where: { $0.requestURL == requestURL })
        {
          BraveGlobalShieldStats.shared.adblock += 1
          let stats = tab.contentBlocker.stats
          tab.contentBlocker.stats = stats.adding(adCount: 1)
          tab.contentBlocker.blockedRequests.append(
            .init(
              requestURL: requestURL,
              sourceURL: windowOriginURL,
              resourceType: dto.data.resourceType,
              isAggressive: domain.globalBlockAdsAndTrackingLevel.isAggressive,
              location: .requestBlocking
            )
          )
        }

        replyHandler(shouldBlock, nil)
      }
    } catch {
      assertionFailure("Invalid type of message. Fix the `RequestBlocking.js` script")
      replyHandler(false, nil)
    }
  }
}
