// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import UIKit

public struct UserAgent {
  /// Mobile user agent for Brave
  public static let mobile = UserAgentBuilder().build(desktopMode: false)
  /// Mobile user agent for masking we are Brave
  public static let mobileMasked = UserAgentBuilder().build(desktopMode: false, maskBrave: true)
  /// Desktop user agent for Brave
  public static let desktop = UserAgentBuilder().build(desktopMode: true)
  /// Desktop user agent for masking we are Brave
  public static let desktopMasked = UserAgentBuilder().build(desktopMode: true, maskBrave: true)

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
