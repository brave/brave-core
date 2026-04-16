// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension BraveUserAgentType {
  /// Returns the user agent that should be used for the `BraveUserAgentType`
  /// in either mobile or desktop mode.
  public func userAgentForMode(isMobile: Bool) -> String {
    switch self {
    case .masked:
      isMobile ? UserAgent.mobileMasked : UserAgent.desktopMasked
    case .version:
      isMobile ? UserAgent.mobile : UserAgent.desktop
    case .suffix:
      isMobile ? UserAgent.safariMobileBraveSuffix : UserAgent.safariDesktopBraveSuffix
    case .suffixComment:
      isMobile
        ? UserAgent.safariMobileBraveSuffixComment : UserAgent.safariDesktopBraveSuffixComment
    @unknown default:
      isMobile ? UserAgent.safariMobileBraveSuffix : UserAgent.safariDesktopBraveSuffix
    }
  }
}
