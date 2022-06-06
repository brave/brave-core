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
    case activeWithPendingRequest
  }
  
  var buttonState: ButtonState = .inactive {
    didSet {
      // We may end up having different states here where active is actually blurple
      tintColor = .braveLabel

      if buttonState == .activeWithPendingRequest {
        addBadgeIfNeeded()
      }
      badgeView.isHidden = buttonState != .activeWithPendingRequest
    }
  }
  
  private let badgeView = UIView()
  private let badgeSize = 10.0
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    adjustsImageWhenHighlighted = false
    setImage(UIImage(named: "menu-crypto", in: .current, compatibleWith: nil)!.template, for: .normal)
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
  
  func addBadgeIfNeeded() {
    guard badgeView.superview == nil else { return }

    badgeView.backgroundColor = .braveErrorBorder
    badgeView.layer.cornerRadius = badgeView.frame.height / 2
    badgeView.layer.masksToBounds = true
    addSubview(badgeView)
    
    if let imageView = imageView {
      badgeView.snp.makeConstraints { make in
        make.size.equalTo(badgeSize)
        make.centerX.equalTo(imageView.snp.trailing)
        make.top.equalTo(imageView.snp.top)
      }
    }
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    badgeView.layer.cornerRadius = badgeView.frame.height / 2
  }
}
