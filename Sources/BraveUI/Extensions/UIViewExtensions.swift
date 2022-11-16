/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

extension UIView {
  /// Creates empty view with specified height or width parameter.
  /// Used mainly to make empty space for UIStackView
  /// Note: on iOS 11+ setCustomSpacing(value, after: View) can be used instead.
  public static func spacer(_ direction: NSLayoutConstraint.Axis, amount: Int) -> UIView {
    let spacer = UIView()
    spacer.snp.makeConstraints { make in
      switch direction {
      case .vertical:
        make.height.equalTo(amount)
      case .horizontal:
        make.width.equalTo(amount)
      @unknown default:
        assertionFailure()
      }
    }
    return spacer
  }
}
