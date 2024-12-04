// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import UIKit

public struct UserAgent {
  public static let mobile = UserAgentBuilder().build(desktopMode: false)
  public static let desktop = UserAgentBuilder().build(desktopMode: true)

  public static func userAgentForIdiom(
    _ idiom: UIUserInterfaceIdiom = UIDevice.current.userInterfaceIdiom
  ) -> String {
    return shouldUseDesktopMode(idiom: idiom) ? UserAgent.desktop : UserAgent.mobile
  }

  public static func shouldUseDesktopMode(
    idiom: UIUserInterfaceIdiom = UIDevice.current.userInterfaceIdiom
  ) -> Bool {
    return idiom == .pad ? Preferences.UserAgent.alwaysRequestDesktopSite.value : false
  }
}
