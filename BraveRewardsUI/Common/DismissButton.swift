/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class DismissButton: Button {
  
  private struct UX {
    static let dismissButtonSize = CGSize(width: 28.0, height: 28.0)
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    setImage(UIImage(frameworkResourceNamed: "close-icon").alwaysTemplate, for: .normal)
    imageView?.tintColor = Colors.grey400
    adjustsImageWhenHighlighted = false
    backgroundColor = UIColor(white: 1.0, alpha: 0.8)
    layer.cornerRadius = UX.dismissButtonSize.width / 2.0
    hitTestSlop = UIEdgeInsets(top: -10.0, left: -10.0, bottom: -10.0, right: -10.0)
    clipsToBounds = true
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override var isHighlighted: Bool {
    didSet {
      if isHighlighted {
        imageView?.tintColor = .white
        backgroundColor = BraveUX.braveOrange
      } else {
        imageView?.tintColor = Colors.grey400
        backgroundColor = UIColor(white: 1.0, alpha: 0.8)
      }
    }
  }
  
  override var intrinsicContentSize: CGSize {
    return UX.dismissButtonSize
  }
}

