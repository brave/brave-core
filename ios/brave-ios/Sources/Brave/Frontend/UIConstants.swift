/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import UIKit

public struct UIConstants {
  static let defaultPadding: CGFloat = 10
  static let snackbarButtonHeight: CGFloat = 48
  static var toolbarHeight: CGFloat = 44
  
  // Static fonts
  static let defaultChromeSize: CGFloat = 16
  static let defaultChromeSmallSize: CGFloat = 11
  static let passcodeEntryFontSize: CGFloat = 36
  static let defaultChromeFont: UIFont = UIFont.systemFont(ofSize: defaultChromeSize, weight: UIFont.Weight.regular)
  static let defaultChromeSmallFontBold = UIFont.boldSystemFont(ofSize: defaultChromeSmallSize)
  static let passcodeEntryFont = UIFont.systemFont(ofSize: passcodeEntryFontSize, weight: UIFont.Weight.bold)

  /// JPEG compression quality for persisted screenshots. Must be between 0-1.
  public static let screenshotQuality: Float = 0.3
  static let activeScreenshotQuality: CGFloat = 0.5
}
