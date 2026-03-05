// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
@_spi(ChromiumWebViewAccess) import Web

extension TabDataValues {
  private struct ForcePasteTabHelperKey: TabDataKey {
    static var defaultValue: ForcePasteTabHelper?
  }
  var forcePaste: ForcePasteTabHelper? {
    get { self[ForcePasteTabHelperKey.self] }
    set { self[ForcePasteTabHelperKey.self] = newValue }
  }
}

class ForcePasteTabHelper {
  private let tab: any TabState
  init?(tab: some TabState) {
    if !tab.isChromiumTab {
      return nil
    }
    self.tab = tab
  }
  /// Pastes the contents passed into the active element on the page
  func forcePasteIntoActiveElement(contents: String) {
    guard let webView = BraveWebView.from(tab: tab) else { return }
    webView.forcePasteContents(contents)
  }
}
