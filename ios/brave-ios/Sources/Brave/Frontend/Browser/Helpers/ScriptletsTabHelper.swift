// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Preferences
@_spi(ChromiumWebViewAccess) import Web

extension TabDataValues {
  private struct ScriptletsTabHelperKey: TabDataKey {
    static var defaultValue: ScriptletsTabHelper?
  }
  public var scriptletsTabHelper: ScriptletsTabHelper? {
    get { self[ScriptletsTabHelperKey.self] }
    set { self[ScriptletsTabHelperKey.self] = newValue }
  }
}

public class ScriptletsTabHelper: TabObserver {

  private weak var tab: (any TabState)?

  public init(
    tab: some TabState
  ) {
    self.tab = tab
    tab.addObserver(self)
  }

  // MARK: - TabObserver

  public func tabDidCreateWebView(_ tab: some TabState) {
    BraveWebView.from(tab: tab)?.setScriptletsTabHelperBridge(self)
  }

  public func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}

@MainActor extension ScriptletsTabHelper: ScriptletsTabHelperBridge {

  public func requestScriptlets(frameURL: URL) async -> [String] {
    guard let tab = tab,
      let tabPageData = tab.currentPageData,
      let braveShieldsHelper = tab.braveShieldsHelper,
      // shield level is determined by the main frame
      case let mainFrameShieldLevel = braveShieldsHelper.shieldLevel(
        for: tabPageData.mainFrameURL,
        considerAllShieldsOption: true
      ),
      mainFrameShieldLevel.isEnabled,
      tabPageData.mainFrameURL.isWebPage(includeDataURIs: false)
    else {
      return []
    }

    let models = await AdBlockGroupsManager.shared.cosmeticFilterModels(
      forFrameURL: frameURL,
      isAdBlockEnabled: mainFrameShieldLevel.isEnabled
    )

    let result: [String] = models.compactMap { tuple in
      let script = tuple.model.injectedScript
      if script.isEmpty {
        return nil
      }

      let isDeAmpEnabled = tab.profile.prefs.boolean(forPath: kDeAmpEnabled)
      return AdblockService.scriptletGlobalsScript(
        withDeAmpEnabled: isDeAmpEnabled
      )
    }
    return result
  }
}
