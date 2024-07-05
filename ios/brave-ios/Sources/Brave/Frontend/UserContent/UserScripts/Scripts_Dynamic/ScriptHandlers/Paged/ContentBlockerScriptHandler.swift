// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Data
import Preferences
import Shared
import WebKit
import os.log

private struct ContentBlockerMessage: Decodable {
  struct Resource: Decodable {
    let resourceType: AdblockEngine.ResourceType
    let resourceURL: String
    let sourceURL: String
  }

  let securityToken: String
  let data: [Resource]
}

private enum BlockedType: Hashable {
  case image
  case ad
}

extension ContentBlockerHelper: TabContentScript {

  static let scriptName = "TrackingProtectionStats"
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
        securityToken: UserScriptManager.securityToken,
        script: script
      ),
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: scriptSandbox
    )
  }()

  func clearPageStats() {
    stats = TPPageStats()
    blockedRequests.removeAll()
  }

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceive message: WKScriptMessage
  ) async -> (Any?, String?) {

    guard let currentTabURL = tab?.webView?.url else {
      assertionFailure("Missing tab or webView")
      return (nil, nil)
    }

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return (nil, nil)
    }

    let messageBody = message.body

    Task.detached { [weak self] in
      guard let self = self else { return }

      let data = try JSONSerialization.data(withJSONObject: messageBody)
      let resources = try JSONDecoder().decode(ContentBlockerMessage.self, from: data).data

      for resource in resources {
        await self.processMessage(resource: resource, currentTabURL: currentTabURL)
      }
    }

    return (nil, nil)
  }

  @MainActor
  private func processMessage(resource: ContentBlockerMessage.Resource, currentTabURL: URL) async {
    let isPrivateBrowsing = self.tab?.isPrivate == true
    let domain = Domain.getOrCreate(forUrl: currentTabURL, persistent: !isPrivateBrowsing)
    if domain.areAllShieldsOff {
      // if domain is "all_off", can just skip
      return
    }

    if resource.resourceType == .script
      && domain.isShieldExpected(.noScript, considerAllShieldsOption: true)
    {
      self.stats = self.stats.adding(scriptCount: 1)
      BraveGlobalShieldStats.shared.scripts += 1
      return
    }

    // Because javascript urls allow some characters that `URL` does not,
    // we use `NSURL(idnString: String)` to parse them
    guard let requestURL = NSURL(idnString: resource.resourceURL) as URL? else { return }
    guard let sourceURL = NSURL(idnString: resource.sourceURL) as URL? else { return }
    guard let domainURLString = domain.url else { return }
    let genericTypes = AdBlockGroupsManager.shared.contentBlockerManager.validGenericTypes(
      for: domain
    )

    let blockedType = await blockedTypes(
      requestURL: requestURL,
      sourceURL: sourceURL,
      enabledRuleTypes: genericTypes,
      resourceType: resource.resourceType,
      domain: domain
    )

    guard let blockedType = blockedType else { return }

    assertIsMainThread("Result should happen on the main thread")

    // Ensure we check that the stats we're tracking is still for the same page
    // Some web pages (like youtube) like to rewrite their main frame urls
    // so we check the source etld+1 agains the tab url etld+1
    // For subframes which may use different etld+1 than the main frame (example `reddit.com` and `redditmedia.com`)
    // We simply check the known subframeURLs on this page.
    guard
      self.tab?.url?.baseDomain == sourceURL.baseDomain
        || self.tab?.currentPageData?.allSubframeURLs.contains(sourceURL) == true
    else {
      return
    }

    if blockedType == .ad, Preferences.PrivacyReports.captureShieldsData.value,
      let domainURL = URL(string: domainURLString),
      let blockedResourceHost = requestURL.baseDomain,
      tab?.isPrivate != true
    {
      PrivacyReportsManager.pendingBlockedRequests.append(
        (blockedResourceHost, domainURL, Date())
      )
    }

    // First check to make sure we're not counting the same repetitive requests multiple times
    guard !self.blockedRequests.contains(requestURL) else { return }
    self.blockedRequests.insert(requestURL)

    // Increase global stats (here due to BlocklistName being in Client and BraveGlobalShieldStats being
    // in BraveShared)
    let stats = BraveGlobalShieldStats.shared
    switch blockedType {
    case .ad:
      stats.adblock += 1
      self.stats = self.stats.adding(adCount: 1)
    case .image:
      stats.images += 1
    }
  }

  @MainActor
  private func blockedTypes(
    requestURL: URL,
    sourceURL: URL,
    enabledRuleTypes: Set<ContentBlockerManager.GenericBlocklistType>,
    resourceType: AdblockEngine.ResourceType,
    domain: Domain
  ) async -> BlockedType? {
    guard let host = requestURL.host, !host.isEmpty else {
      // TP Stats init isn't complete yet
      return nil
    }

    if resourceType == .image && Preferences.Shields.blockImages.value {
      return .image
    }

    if enabledRuleTypes.contains(.blockAds) || enabledRuleTypes.contains(.blockTrackers) {
      if await AdBlockGroupsManager.shared.shouldBlock(
        requestURL: requestURL,
        sourceURL: sourceURL,
        resourceType: resourceType,
        domain: domain
      ) {
        return .ad
      }
    }

    return nil
  }
}
