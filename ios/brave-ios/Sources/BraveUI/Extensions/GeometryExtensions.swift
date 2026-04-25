// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

extension CGRect {
  public var center: CGPoint {
    get {
      return CGPoint(x: size.width / 2, y: size.height / 2)
    }
    set {
      self.origin = CGPoint(x: newValue.x - size.width / 2, y: newValue.y - size.height / 2)
    }
  }
}

extension UIEdgeInsets {
  public init(equalInset inset: CGFloat) {
    self.init()
    top = inset
    left = inset
    right = inset
    bottom = inset
  }

  public init(vertical: CGFloat, horizontal: CGFloat) {
    self.init(
      top: vertical,
      left: horizontal,
      bottom: vertical,
      right: horizontal
    )
  }
}
