// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import SwiftUI
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

  public static let chromeMobile = """
    Mozilla/5.0 (iPhone; CPU iPhone OS 18_3_1 like Mac OS X) \
    AppleWebKit/605.1.15 (KHTML, like Gecko) \
    CriOS/140.0.7339.101 Mobile/15E148 Safari/604.1
    """
  public static let chromeDesktop = """
    Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
    AppleWebKit/605.1.15 (KHTML, like Gecko) \
    CriOS/140 Version/11.1.1 Safari/605.1.15
    """
  public static let chromeMobileBraveComment = """
    Mozilla/5.0 (iPhone; CPU iPhone OS 18_3_1 like Mac OS X) \
    AppleWebKit/605.1.15 (KHTML, like Gecko) \
    CriOS (Brave)/140.0.7339.101 Mobile/15E148 Safari/604.1
    """
  public static let chromeDesktopBraveComment = """
    Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
    AppleWebKit/605.1.15 (KHTML, like Gecko) \
    CriOS (Brave)/140 Version/11.1.1 Safari/605.1.15
    """
}
