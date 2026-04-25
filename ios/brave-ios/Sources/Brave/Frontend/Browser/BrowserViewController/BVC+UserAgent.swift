// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import UserAgent
import Web

extension BrowserViewController {

  public func userAgent(
    for request: URLRequest,
    userAgentForType type: UserAgentType,
    braveUserAgentExceptions: BraveUserAgentExceptionsIOS?
  ) -> String {
    if !Preferences.Debug.userAgentOverride.value.isEmpty {
      return Preferences.Debug.userAgentOverride.value
    }
    let isBraveAllowedInUA =
      request.mainDocumentURL.flatMap {
        braveUserAgentExceptions?.canShowBrave($0)
      } ?? true

    let mobile: String
    let desktop: String
    if isBraveAllowedInUA {
      let userAgentType = GetDefaultBraveIOSUserAgentType()
      mobile = userAgentType.userAgentForMode(isMobile: true)
      desktop = userAgentType.userAgentForMode(isMobile: false)
    } else {
      mobile = UserAgent.mobileMasked
      desktop = UserAgent.desktopMasked
    }

    switch type {
    case .none, .automatic:
      let screenWidth = UIScreen.main.bounds.width
      if traitCollection.horizontalSizeClass == .compact && view.bounds.width < screenWidth / 2 {
        return mobile
      }
      return traitCollection.userInterfaceIdiom == .pad
        && profileController.defaultHostContentSettings.defaultPageMode == .desktop
        ? desktop : mobile
    case .desktop: return desktop
    case .mobile: return mobile
    }
  }
}
