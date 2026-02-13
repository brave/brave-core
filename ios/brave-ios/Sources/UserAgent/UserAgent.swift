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
  public static let mobileMasked = UserAgentBuilder().build(
    desktopMode: false,
    useSafariUA: true
  )
  /// Desktop user agent for Brave
  public static let desktop = UserAgentBuilder().build(desktopMode: true)
  /// Desktop user agent for masking we are Brave
  public static let desktopMasked = UserAgentBuilder().build(
    desktopMode: true,
    useSafariUA: true
  )
}
