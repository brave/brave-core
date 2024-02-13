// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import Shared

extension BrowserViewController {
  func launchNativeBraveTalk(tab: Tab?, room: String, token: String) {
#if canImport(BraveTalk)
    braveTalkJitsiCoordinator.launchNativeBraveTalk(
      for: room,
      token: token,
      onEnterCall: { [weak tab] in
        tab?.webView?.stopLoading()
      },
      onExitCall: { [weak self] in
        guard let self = self else { return }
        // When we close the call, redirect to Brave Talk home page if the selected tab is still the original
        // talk URL
        if let url = self.tabManager.selectedTab?.url,
           let currentHost = url.host,
           DomainUserScript.braveTalkHelper.associatedDomains.contains(currentHost) {
          var components = URLComponents()
          components.host = currentHost
          components.scheme = url.scheme
          self.select(url: components.url!, isUserDefinedURLNavigation: false)
        }
      }
    )
#endif
  }
}
