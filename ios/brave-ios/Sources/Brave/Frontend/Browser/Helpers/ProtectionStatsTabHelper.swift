// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Data
import Preferences
import Shared
@_spi(ChromiumWebViewAccess) import Web
import os.log

extension TabDataValues {
  private struct ProtectionStatsTabHelperKey: TabDataKey {
    static var defaultValue: ProtectionStatsTabHelper?
  }
  var protectionStats: ProtectionStatsTabHelper? {
    get { self[ProtectionStatsTabHelperKey.self] }
    set { self[ProtectionStatsTabHelperKey.self] = newValue }
  }
}

/// Tracks the resources loaded by a page so Shields can record blocked
/// trackers, ads, scripts and images. The stats logic is shared between the
/// legacy `TrackingProtectionStats` script handler and the
/// `ProtectionStatsJavaScriptFeature` bridge.
@MainActor
class ProtectionStatsTabHelper: TabObserver, @preconcurrency ProtectionStatsTabHelperBridge {
  /// A single resource reported by the page.
  struct BlockedResource {
    let resourceURL: String
    let resourceType: AdblockEngine.ResourceType
  }

  private enum BlockedType: Hashable {
    case image
    case ad
  }

  private weak var tab: (any TabState)?

  init(tab: some TabState) {
    self.tab = tab

    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  // MARK: - TabObserver

  func tabDidCreateWebView(_ tab: some TabState) {
    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      BraveWebView.from(tab: tab)?.protectionStatsHelper = self
    }
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: - ProtectionStatsTabHelperBridge

  func handleBlockedResources(
    _ resources: [ProtectionStatsResource],
    securityOrigin: URL
  ) {
    let blockedResources = resources.compactMap { resource -> BlockedResource? in
      guard let resourceType = AdblockEngine.ResourceType(rawValue: resource.resourceType)
      else {
        return nil
      }
      return BlockedResource(resourceURL: resource.resourceURL, resourceType: resourceType)
    }

    handleBlockedResources(blockedResources, sourceURL: securityOrigin)
  }

  // MARK: -

  /// Processes the resources reported by `sourceURL`, updating the tab's
  /// content blocker stats and the global Shields stats for any resource that
  /// is determined to be blocked.
  func handleBlockedResources(_ resources: [BlockedResource], sourceURL: URL) {
    guard let tab, let currentTabURL = tab.visibleURL else {
      return
    }

    for resource in resources {
      Task { @MainActor in
        guard let braveShieldsHelper = tab.braveShieldsHelper else { return }
        if !braveShieldsHelper.isBraveShieldsEnabled(for: currentTabURL) {
          // if shields are disabled, can return early.
          return
        }

        if resource.resourceType == .script
          && braveShieldsHelper.isShieldExpected(
            for: currentTabURL,
            shield: .noScript,
            considerAllShieldsOption: true
          )
        {
          if let contentBlocker = tab.contentBlocker {
            contentBlocker.stats = contentBlocker.stats.adding(scriptCount: 1)
          }
          BraveGlobalShieldStats.shared.scripts += 1
          return
        }

        // Because javascript urls allow some characters that `URL` does not,
        // we use `NSURL(idnString: String)` to parse them
        guard let requestURL = NSURL(idnString: resource.resourceURL) as URL? else {
          return
        }

        let shieldLevel = braveShieldsHelper.shieldLevel(
          for: currentTabURL,
          considerAllShieldsOption: true
        )
        let genericTypes = AdBlockGroupsManager.shared.contentBlockerManager.validGenericTypes(
          isShieldsEnabled: braveShieldsHelper.isBraveShieldsEnabled(for: currentTabURL),
          isAdBlockEnabled: shieldLevel.isEnabled,
          isBlockAllCookiesEnabled: tab.profile.prefs.boolean(forPath: kBlockAllCookiesEnabled)
        )

        let blockedType = await blockedType(
          requestURL: requestURL,
          sourceURL: sourceURL,
          enabledRuleTypes: genericTypes,
          resourceType: resource.resourceType,
          isAdBlockEnabled: shieldLevel.isEnabled,
          isAdBlockModeAggressive: shieldLevel.isAggressive
        )

        guard let blockedType = blockedType else { return }

        // Ensure we check that the stats we're tracking is still for the same page
        // Some web pages (like youtube) like to rewrite their main frame urls
        // so we check the source etld+1 against the tab url etld+1
        // For subframes which may use different etld+1 than the main frame (example `reddit.com` and `redditmedia.com`)
        // We simply check the known subframeURLs on this page.
        let subframeBaseDomains = (tab.currentPageData?.allSubframeURLs ?? [])
          .compactMap(\.baseDomain)
        guard let sourceURLBaseDomain = sourceURL.baseDomain,
          tab.visibleURL?.baseDomain == sourceURLBaseDomain
            || subframeBaseDomains.contains(sourceURLBaseDomain) == true
        else {
          return
        }

        if blockedType == .ad, Preferences.PrivacyReports.captureShieldsData.value,
          let blockedResourceHost = requestURL.baseDomain,
          !tab.isPrivate
        {
          PrivacyReportsManager.pendingBlockedRequests.append(
            (blockedResourceHost, currentTabURL.domainURL, Date())
          )
        }

        guard let contentBlocker = tab.contentBlocker else { return }

        // First check to make sure we're not counting the same repetitive requests multiple times
        guard !contentBlocker.blockedRequests.contains(where: { $0.requestURL == requestURL })
        else {
          return
        }
        contentBlocker.blockedRequests.append(
          .init(
            requestURL: requestURL,
            sourceURL: sourceURL,
            resourceType: resource.resourceType,
            isAggressive: shieldLevel.isAggressive,
            location: .contentBlocker
          )
        )

        // Increase global stats (here due to BlocklistName being in Client and
        // BraveGlobalShieldStats being in BraveShared)
        let stats = BraveGlobalShieldStats.shared
        switch blockedType {
        case .ad:
          stats.adblock += 1
          contentBlocker.stats = contentBlocker.stats.adding(adCount: 1)
        case .image:
          stats.images += 1
        }
      }
    }
  }

  private func blockedType(
    requestURL: URL,
    sourceURL: URL,
    enabledRuleTypes: Set<ContentBlockerManager.GenericBlocklistType>,
    resourceType: AdblockEngine.ResourceType,
    isAdBlockEnabled: Bool,
    isAdBlockModeAggressive: Bool
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
        isAdBlockEnabled: isAdBlockEnabled,
        isAdBlockModeAggressive: isAdBlockModeAggressive
      ) {
        return .ad
      }
    }

    return nil
  }
}
