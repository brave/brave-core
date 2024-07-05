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

private struct RequestBlockingMessage: Decodable {
  struct Resource: Decodable, Hashable {
    let resourceType: AdblockEngine.ResourceType
    let resourceURL: String
    let sourceURL: String
  }

  let securityToken: String
  let data: Resource
}

class RequestBlockingContentScriptHandler: TabContentScript {

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

  private weak var tab: Tab?

  init(tab: Tab) {
    self.tab = tab
  }

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceive message: WKScriptMessage
  ) async -> (Any?, String?) {
    guard let tab = tab, let currentTabURL = tab.webView?.url else {
      assertionFailure("Should have a tab set")
      return (nil, nil)
    }

    if !verifyMessage(message: message) {
      assertionFailure("Invalid security token. Fix the `RequestBlocking.js` script")
      return (false, nil)
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let message = try JSONDecoder().decode(RequestBlockingMessage.self, from: data)

      // Because javascript urls allow some characters that `URL` does not,
      // we use `NSURL(idnString: String)` to parse them
      guard let requestURL = NSURL(idnString: message.data.resourceURL) as URL? else {
        return (nil, nil)
      }
      guard let sourceURL = NSURL(idnString: message.data.sourceURL) as URL? else {
        return (nil, nil)
      }

      return await processResource(
        message.data,
        tab: tab,
        tabURL: currentTabURL,
        requestURL: requestURL,
        sourceURL: sourceURL
      )
    } catch {
      assertionFailure("Invalid type of message. Fix the `RequestBlocking.js` script")
      return (false, nil)
    }
  }

  @MainActor
  private func processResource(
    _ resource: RequestBlockingMessage.Resource,
    tab: Tab,
    tabURL: URL,
    requestURL: URL,
    sourceURL: URL
  ) async -> (Any?, String?) {
    let domain = Domain.getOrCreate(forUrl: tabURL, persistent: !tab.isPrivate)
    guard let domainURLString = domain.url else { return (nil, nil) }
    let shouldBlock = await AdBlockGroupsManager.shared.shouldBlock(
      requestURL: requestURL,
      sourceURL: sourceURL,
      resourceType: resource.resourceType,
      domain: domain
    )

    // Ensure we check that the stats we're tracking is still for the same page
    // Some web pages (like youtube) like to rewrite their main frame urls
    // so we check the source etld+1 agains the tab url etld+1
    // For subframes which may use different etld+1 than the main frame (example `reddit.com` and `redditmedia.com`)
    // We simply check the known subframeURLs on this page.
    guard
      tab.url?.baseDomain == sourceURL.baseDomain
        || tab.currentPageData?.allSubframeURLs.contains(sourceURL) == true
    else {
      return (shouldBlock, nil)
    }

    if shouldBlock, Preferences.PrivacyReports.captureShieldsData.value,
      let domainURL = URL(string: domainURLString),
      let blockedResourceHost = requestURL.baseDomain,
      !tab.isPrivate
    {
      PrivacyReportsManager.pendingBlockedRequests.append(
        (blockedResourceHost, domainURL, Date())
      )
    }

    if shouldBlock && !tab.contentBlocker.blockedRequests.contains(requestURL) {
      BraveGlobalShieldStats.shared.adblock += 1
      let stats = tab.contentBlocker.stats
      tab.contentBlocker.stats = stats.adding(adCount: 1)
      tab.contentBlocker.blockedRequests.insert(requestURL)
    }

    return (shouldBlock, nil)
  }
}
