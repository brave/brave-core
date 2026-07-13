// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Web

extension TabDataValues {
  private struct DetachedTabPrivacyHelperKey: TabDataKey {
    static var defaultValue: DetachedTabPrivacyHelper?
  }
  var detachedPrivacyHelper: DetachedTabPrivacyHelper? {
    get { self[DetachedTabPrivacyHelperKey.self] }
    set { self[DetachedTabPrivacyHelperKey.self] = newValue }
  }
}

/// A tab helper that sets up shields & privacy related features for a TabState that wont be
/// added to the TabManager/associated with BrowserViewController
@MainActor
class DetachedTabPrivacyHelper: TabPolicyDecider {
  weak var tab: (any TabState)?

  convenience init?(tab: some TabState) {
    guard
      let shieldSettings = BraveShieldsSettingsServiceFactory.get(profile: tab.profile)
    else { return nil }
    self.init(
      tab: tab,
      shieldsSettings: shieldSettings
    )
  }

  init(
    tab: some TabState,
    shieldsSettings: any BraveShieldsSettings
  ) {
    self.tab = tab

    tab.addPolicyDecider(self)

    // for now we need to attach the entire browser data for shields to work
    if tab.browserData == nil {
      tab.browserData = TabBrowserData(tab: tab)
    }
    var privacyRelatedScripts: [TabContentScript] = [
      RequestBlockingContentScriptHandler(),
      SiteStateListenerScriptHandler(),
      CosmeticFiltersScriptHandler(),
    ]
    if let contentBlocker = tab.browserData?.contentBlocker {
      privacyRelatedScripts.append(contentBlocker)
    }
    for script in privacyRelatedScripts {
      tab.browserData?.addContentScript(
        script,
        name: type(of: script).scriptName,
        contentWorld: type(of: script).scriptSandbox
      )
    }
    let shieldsHelper = BraveShieldsTabHelper(tab: tab, braveShieldsSettings: shieldsSettings)
    tab.braveShieldsHelper = shieldsHelper
    tab.addPolicyDecider(shieldsHelper)
  }

  // MARK: - TabPolicyDecider

  func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard let mainDocumentURL = request.mainDocumentURL else { return .allow }

    let pageData = PageData(mainFrameURL: mainDocumentURL)
    if mainDocumentURL != tab.currentPageData?.mainFrameURL {
      tab.currentPageData = pageData
    }

    let isAdBlockEnabled =
      tab.braveShieldsHelper?.shieldLevel(
        for: mainDocumentURL,
        considerAllShieldsOption: true
      ).isEnabled ?? true
    let isBlockFingerprintingEnabled =
      tab.braveShieldsHelper?.isShieldExpected(
        for: mainDocumentURL,
        shield: .fpProtection,
        considerAllShieldsOption: true
      ) ?? true

    if requestInfo.isMainFrame {
      tab.browserData?.setScripts(scripts: [
        // Add de-amp script
        // The user script manager will take care to not reload scripts if this value doesn't change
        .deAmp: tab.profile.prefs.boolean(forPath: kDeAmpEnabled),

        // Add request blocking script
        // This script will block certian `xhr` and `window.fetch()` requests
        .requestBlocking: mainDocumentURL.isWebPage(includeDataURIs: false)
          && isAdBlockEnabled,

        // The tracker protection script
        // This script will track what is blocked and increase stats
        .trackerProtectionStats: mainDocumentURL.isWebPage(includeDataURIs: false)
          && isAdBlockEnabled,
      ])
    }

    let scriptTypes =
      await tab.currentPageData?.makeUserScriptTypes(
        isPrivateBrowsing: tab.isPrivate,
        isDeAmpEnabled: tab.profile.prefs.boolean(forPath: kDeAmpEnabled),
        isAdBlockEnabled: isAdBlockEnabled,
        isBlockFingerprintingEnabled: isBlockFingerprintingEnabled,
        isGPCEnabled: tab.profile.prefs.boolean(forPath: kGlobalPrivacyControlEnabled)
      ) ?? []
    tab.browserData?.setCustomUserScript(scripts: scriptTypes)

    if !requestInfo.isNewWindow, let requestURL = request.url {
      // Check if custom user scripts must be added to or removed from the web view.
      tab.currentPageData?.addSubframeURL(
        forRequestURL: requestURL,
        isForMainFrame: requestInfo.isMainFrame
      )
      let scriptTypes =
        await tab.currentPageData?.makeUserScriptTypes(
          isPrivateBrowsing: tab.isPrivate,
          isDeAmpEnabled: tab.profile.prefs.boolean(forPath: kDeAmpEnabled),
          isAdBlockEnabled: isAdBlockEnabled,
          isBlockFingerprintingEnabled: isBlockFingerprintingEnabled,
          isGPCEnabled: tab.profile.prefs.boolean(
            forPath: kGlobalPrivacyControlEnabled
          )
        ) ?? []
      tab.browserData?.setCustomUserScript(scripts: scriptTypes)
    }

    return .allow
  }

  func tab(
    _ tab: some TabState,
    shouldAllowResponse response: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision {
    // Check if we upgraded to https and if so we need to update the url of frame evaluations
    if let responseURL = response.url,
      let pageData = tab.currentPageData,
      tab.currentPageData?.upgradeFrameURL(
        forResponseURL: responseURL,
        isForMainFrame: responseInfo.isForMainFrame
      ) == true
    {
      let scriptTypes =
        await tab.currentPageData?.makeUserScriptTypes(
          isPrivateBrowsing: tab.isPrivate,
          isDeAmpEnabled: tab.profile.prefs.boolean(forPath: kDeAmpEnabled),
          isAdBlockEnabled: tab.braveShieldsHelper?.shieldLevel(
            for: pageData.mainFrameURL,
            considerAllShieldsOption: true
          ).isEnabled ?? true,
          isBlockFingerprintingEnabled: tab.braveShieldsHelper?.isShieldExpected(
            for: pageData.mainFrameURL,
            shield: .fpProtection,
            considerAllShieldsOption: true
          ) ?? true,
          isGPCEnabled: tab.profile.prefs.boolean(
            forPath: kGlobalPrivacyControlEnabled
          )
        ) ?? []
      tab.browserData?.setCustomUserScript(scripts: scriptTypes)
    }

    return .allow
  }
}
