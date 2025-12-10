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

  private let deAmpPrefs: DeAmpPrefs

  convenience init?(tab: some TabState, profileController: BraveProfileController) {
    let profile =
      tab.isPrivate ? profileController.profile.offTheRecordProfile : profileController.profile
    guard
      let shieldSettings = BraveShieldsSettingsServiceFactory.get(profile: profile)
    else { return nil }
    self.init(
      tab: tab,
      shieldsSettings: shieldSettings,
      deAmpPrefs: profileController.deAmpPrefs
    )
  }

  init(
    tab: some TabState,
    shieldsSettings: any BraveShieldsSettings,
    deAmpPrefs: DeAmpPrefs
  ) {
    self.tab = tab
    self.deAmpPrefs = deAmpPrefs

    tab.addPolicyDecider(self)

    // for now we need to attach the entire browser data for shields to work
    let browserData = TabBrowserData(tab: tab)
    tab.browserData = browserData
    let privacyRelatedScripts: [TabContentScript] = [
      RequestBlockingContentScriptHandler(),
      SiteStateListenerScriptHandler(),
      CosmeticFiltersScriptHandler(),
      browserData.contentBlocker,
    ]
    for script in privacyRelatedScripts {
      browserData.addContentScript(
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
    if let mainDocumentURL = request.mainDocumentURL {
      let pageData = PageData(mainFrameURL: mainDocumentURL)
      if mainDocumentURL != tab.currentPageData?.mainFrameURL {
        tab.currentPageData = pageData
      }

      let scriptTypes =
        await tab.currentPageData?.makeUserScriptTypes(
          isDeAmpEnabled: deAmpPrefs.isDeAmpEnabled,
          isAdBlockEnabled: tab.braveShieldsHelper?.shieldLevel(
            for: pageData.mainFrameURL,
            considerAllShieldsOption: true
          ).isEnabled ?? true,
          isBlockFingerprintingEnabled: tab.braveShieldsHelper?.isShieldExpected(
            for: pageData.mainFrameURL,
            shield: .fpProtection,
            considerAllShieldsOption: true
          ) ?? true
        ) ?? []
      tab.browserData?.setCustomUserScript(scripts: scriptTypes)
    }

    return .allow
  }
}
