// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

/// A UIView whos layer is a gradient layer
public class GradientView: UIView {
  
  public convenience init() {
    self.init(colors: [], positions: [], startPoint: .zero, endPoint: CGPoint(x: 0, y: 1))
  }
  
  public init(colors: [UIColor], positions: [CGFloat], startPoint: CGPoint, endPoint: CGPoint) {
    super.init(frame: .zero)
    
    gradientLayer.colors = colors.map { $0.cgColor }
    gradientLayer.locations = positions.map { NSNumber(value: Double($0)) }
    gradientLayer.startPoint = startPoint
    gradientLayer.endPoint = endPoint
  }
  
  public init(colors: [UIColor], positions: [CGFloat], angle: Float = 0) {
    super.init(frame: .zero)
    
    // x² + y² = r² for unit circles
    // Constraints:
    // y = x * tan(angle) { 0 < x < cos(a) }
    // y = x * tan(angle) { cos(a) < x < 0 }
    // x = cos(a) { x < y < sin(a) }
    // x = cos(a) { sin(a) < y < 0 }
        
    let alpha = angle * .pi / 180.0
    let startPointX = 0.5 * sin(alpha) + 0.5
    let startPointY = 0.5 * cos(alpha) + 0.5
    let endPointX = -0.5 * sin(alpha) + 0.5
    let endPointY = -0.5 * cos(alpha) + 0.5
    
    gradientLayer.colors = colors.map { $0.cgColor }
    gradientLayer.locations = positions.map { NSNumber(value: Double($0)) }
    gradientLayer.startPoint = CGPoint(x: CGFloat(startPointX), y: CGFloat(startPointY))
    gradientLayer.endPoint = CGPoint(x: CGFloat(endPointX), y: CGFloat(endPointY))
  }
    
  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }
    
  /// The gradient layer which you may modify
  public var gradientLayer: CAGradientLayer {
    return layer as! CAGradientLayer // swiftlint:disable:this force_cast
  }
    
  public override class var layerClass: AnyClass {
    return CAGradientLayer.self
  }
}

