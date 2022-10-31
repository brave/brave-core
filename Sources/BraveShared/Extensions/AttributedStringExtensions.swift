/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit

// MARK: - NSAttributedString

extension NSAttributedString {

  /// Add Line Spacing to Text
  public func withLineSpacing(_ spacing: CGFloat) -> NSAttributedString {
    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.lineSpacing = spacing

    let attributedString = NSMutableAttributedString(attributedString: self)
    attributedString.addAttribute(
      .paragraphStyle,
      value: paragraphStyle,
      range: NSRange(location: 0, length: string.count))

    return attributedString
  }
}
