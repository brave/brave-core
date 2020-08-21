// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

/// A control that scales down when highlighted and back up when not
open class SpringButton: UIControl {
  /// The scale to adjust to when the control is highlighted
  open var highlightScale: CGFloat = 0.95
  
  override open var isHighlighted: Bool {
    didSet {
      UIViewPropertyAnimator(duration: 0.3, dampingRatio: 0.8) {
        let scale = self.isHighlighted ? self.highlightScale : 1.0
        self.transform = CGAffineTransform(scaleX: scale, y: scale)
      }
      .startAnimation()
    }
  }
  
  public override init(frame: CGRect) {
    super.init(frame: frame)
    
    accessibilityTraits.insert(.button)
    isAccessibilityElement = true
  }
  
  @available(*, unavailable)
  public required init(coder: NSCoder) {
    fatalError()
  }
}
