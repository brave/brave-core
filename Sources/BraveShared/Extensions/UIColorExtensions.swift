/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit

extension UIColor {

  public func toHexString() -> String {
    var r: CGFloat = 0
    var g: CGFloat = 0
    var b: CGFloat = 0
    var a: CGFloat = 0

    getRed(&r, green: &g, blue: &b, alpha: &a)

    let rgb: Int = (Int)(r * 255) << 16 | (Int)(g * 255) << 8 | (Int)(b * 255) << 0

    return String(format: "%06x", rgb)
  }

  public var isLight: Bool {
    var white: CGFloat = 0
    getWhite(&white, alpha: nil)
    return white > 0.5
  }

  public var slightlyDesaturated: UIColor {
    desaturated(0.5)
  }

  public func desaturated(_ desaturation: CGFloat) -> UIColor {
    var h: CGFloat = 0, s: CGFloat = 0
    var b: CGFloat = 0, a: CGFloat = 0

    guard self.getHue(&h, saturation: &s, brightness: &b, alpha: &a) else {
      return self
    }

    return UIColor(
      hue: h,
      saturation: max(s - desaturation, 0.0),
      brightness: b,
      alpha: a)
  }
}
