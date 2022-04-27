// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

class WalletURLBarButton: UIButton {
  
  enum ButtonState {
    case inactive
    case active
  }
  
  var buttonState: ButtonState = .inactive {
    didSet {
      isHidden = buttonState == .inactive
      // We may end up having different states here where active is actually blurple
      tintColor = .braveLabel
    }
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    adjustsImageWhenHighlighted = false
    setImage(UIImage(imageLiteralResourceName: "menu-crypto").template, for: .normal)
    imageView?.contentMode = .scaleAspectFit
    imageEdgeInsets = .init(top: 3, left: 3, bottom: 3, right: 3)
  }
  
  override open var isHighlighted: Bool {
    didSet {
      self.tintColor = isHighlighted ? .braveOrange : .braveLabel
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
