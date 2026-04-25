// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

public class NotificationBorderView: UIView {

  public var didClickBorderedArea: (() -> Void)?

  public init(
    frame: CGRect,
    cornerRadius: CGFloat,
    lineWidth: CGFloat = 2,
    colouredBorder: Bool = false
  ) {
    let borderLayer = CAShapeLayer().then {
      let frame = frame.with { $0.origin = .zero }
      $0.strokeColor = colouredBorder ? UIColor.braveLighterBlurple.cgColor : UIColor.white.cgColor
      $0.fillColor = UIColor.clear.cgColor
      $0.lineWidth = lineWidth
      $0.strokeEnd = 1.0
      $0.path = UIBezierPath(roundedRect: frame, cornerRadius: cornerRadius).cgPath
    }

    super.init(frame: frame)
    layer.addSublayer(borderLayer)

    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onClickBorder(_:))))
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  @objc
  private func onClickBorder(_ tap: UITapGestureRecognizer) {
    didClickBorderedArea?()
  }
}
