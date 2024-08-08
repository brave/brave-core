// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import UIKit

// FIXME: Delete this, expose WebClient's UserAgent methods from BraveCore instead
@available(*, deprecated)
public struct UserAgent {
  public static let mobile = UserAgentBuilder().build(desktopMode: false)
  public static let desktop = UserAgentBuilder().build(desktopMode: true)

  public static var userAgentForDesktopMode: String {
    UserAgent.shouldUseDesktopMode ? UserAgent.desktop : UserAgent.mobile
  }

  public static var shouldUseDesktopMode: Bool {
    if UIDevice.current.userInterfaceIdiom == .pad {
      return Preferences.UserAgent.alwaysRequestDesktopSite.value
    }
    return false
  }
}
