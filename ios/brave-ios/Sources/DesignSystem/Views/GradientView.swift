// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import UIKit

/// A UIView whos layer is a gradient layer
public class GradientView: UIView {

  public convenience init() {
    self.init(colors: [], positions: [], startPoint: .zero, endPoint: CGPoint(x: 0, y: 1))
  }

  public init(colors: [UIColor], positions: [CGFloat], startPoint: CGPoint, endPoint: CGPoint) {
    super.init(frame: .zero)

    gradientLayer.colors = colors.map { $0.resolvedColor(with: traitCollection).cgColor }
    gradientLayer.locations = positions.map { NSNumber(value: Double($0)) }
    gradientLayer.startPoint = startPoint
    gradientLayer.endPoint = endPoint
  }

  /// The gradient layer which you may modify
  public var gradientLayer: CAGradientLayer {
    return layer as! CAGradientLayer
  }

  public override class var layerClass: AnyClass {
    return CAGradientLayer.self
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }
}
