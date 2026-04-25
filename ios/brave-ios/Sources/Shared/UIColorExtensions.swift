// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

extension UIColor {
  /// Initializes and returns a color object for the given RGB hex integer.
  public convenience init(rgb: Int) {
    self.init(
      red: CGFloat((rgb & 0xFF0000) >> 16) / 255.0,
      green: CGFloat((rgb & 0x00FF00) >> 8) / 255.0,
      blue: CGFloat((rgb & 0x0000FF) >> 0) / 255.0,
      alpha: 1
    )
  }

  public convenience init(rgba: UInt32) {
    self.init(
      red: CGFloat((rgba & 0xFF00_0000) >> 24) / 255.0,
      green: CGFloat((rgba & 0x00FF_0000) >> 16) / 255.0,
      blue: CGFloat((rgba & 0x0000_FF00) >> 8) / 255.0,
      alpha: CGFloat((rgba & 0x0000_00FF) >> 0) / 255.0
    )
  }

  public convenience init(colorString: String) {
    let string = colorString.replacingOccurrences(of: "#", with: "")

    let colorInt = UInt32(Scanner(string: string).scanInt32(representation: .hexadecimal) ?? 0)
    self.init(rgb: (Int)(colorInt))
  }

  public var rgba: UInt {
    var (r, g, b, a): (CGFloat, CGFloat, CGFloat, CGFloat) = (0.0, 0.0, 0.0, 0.0)
    getRed(&r, green: &g, blue: &b, alpha: &a)
    return (UInt(r * 255.0) << 24) | (UInt(g * 255.0) << 16) | (UInt(b * 255.0) << 8)
      | UInt(a * 255.0)
  }
}
