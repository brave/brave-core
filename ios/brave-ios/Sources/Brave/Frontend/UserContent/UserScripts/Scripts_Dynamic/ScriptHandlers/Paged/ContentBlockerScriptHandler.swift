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

extension ContentBlockerHelper: TabContentScript {
  private struct ContentBlockerDTO: Decodable {
    struct ContentblockerDTOData: Decodable {
      let resourceType: AdblockEngine.ResourceType
      let resourceURL: String
      let sourceURL: String
    }

    let securityToken: String
    let data: [ContentblockerDTOData]
  }

  enum BlockedType: Hashable {
    case image
    case ad
  }

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
    didReceiveScriptMessage message: WKScriptMessage,
    replyHandler: (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    guard let currentTabURL = tab?.webView?.url else {
      assertionFailure("Missing tab or webView")
      return
    }

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dtos = try JSONDecoder().decode(ContentBlockerDTO.self, from: data).data

      dtos.forEach { dto in
        Task { @MainActor in
          let isPrivateBrowsing = self.tab?.isPrivate == true
          let domain = Domain.getOrCreate(forUrl: currentTabURL, persistent: !isPrivateBrowsing)
          if domain.areAllShieldsOff {
            // if domain is "all_off", can just skip
            return
          }

          if dto.resourceType == .script
            && domain.isShieldExpected(.noScript, considerAllShieldsOption: true)
          {
            self.stats = self.stats.adding(scriptCount: 1)
            BraveGlobalShieldStats.shared.scripts += 1
            return
          }

          // Because javascript urls allow some characters that `URL` does not,
          // we use `NSURL(idnString: String)` to parse them
          guard let requestURL = NSURL(idnString: dto.resourceURL) as URL? else { return }
          guard let sourceURL = NSURL(idnString: dto.sourceURL) as URL? else { return }
          guard let domainURLString = domain.url else { return }
          let genericTypes = AdBlockGroupsManager.shared.contentBlockerManager.validGenericTypes(
            for: domain
          )

          let blockedType = await blockedTypes(
            requestURL: requestURL,
            sourceURL: sourceURL,
            enabledRuleTypes: genericTypes,
            resourceType: dto.resourceType,
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
          guard !self.blockedRequests.contains(where: { $0.requestURL == requestURL }) else {
            return
          }
          self.blockedRequests.append(
            .init(
              requestURL: requestURL,
              sourceURL: sourceURL,
              resourceType: dto.resourceType,
              isAggressive: domain.globalBlockAdsAndTrackingLevel.isAggressive,
              location: .contentBlocker
            )
          )

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
      }
    } catch {
      Logger.module.error("\(error.localizedDescription)")
    }
  }

  @MainActor func blockedTypes(
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
