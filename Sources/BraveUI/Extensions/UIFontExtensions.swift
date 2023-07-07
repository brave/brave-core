/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

extension UIFont {
  /// Extended Dynamic Font Function for custom weight
  /// - Parameters:
  ///   - style: textStyle for the dynamic font
  ///   - weight: weight of the dynamic font to be adjusted
  public static func preferredFont(for style: TextStyle, weight: Weight, traitCollection: UITraitCollection? = nil) -> UIFont {
    let fontMetrics = UIFontMetrics(forTextStyle: style)
    let fontDescriptor = UIFontDescriptor.preferredFontDescriptor(withTextStyle: style, compatibleWith: traitCollection)
    let font = UIFont.systemFont(ofSize: fontDescriptor.pointSize, weight: weight)

    return fontMetrics.scaledFont(for: font)
  }

  public func with(traits: UIFontDescriptor.SymbolicTraits?) -> UIFont {
    guard let traits = traits, let descriptor = fontDescriptor.withSymbolicTraits(traits) else {
      return self
    }

    return UIFont(descriptor: descriptor, size: 0)
  }
}
