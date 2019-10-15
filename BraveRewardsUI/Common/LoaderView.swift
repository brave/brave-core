/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

private class LoaderLayer: CALayer {
  weak var parent: LoaderView?
  
  override func action(forKey event: String) -> CAAction? {
    if event == kCAOnOrderIn && parent?.isAnimating == true {
      // Resume animation
      parent?.start()
    }
    return super.action(forKey: event)
  }
}

/// A Brave styled activity indicator view
class LoaderView: UIView {
  /// The size of the indicator
  enum Size {
    case small
    case normal
    
    fileprivate var size: CGSize {
      switch self {
      case .small:
        return CGSize(width: 16, height: 16)
      case .normal:
        return CGSize(width: 32, height: 32)
      }
    }
    
    fileprivate var lineWidth: CGFloat {
      switch self {
      case .small:
        return 2.0
      case .normal:
        return 3.0
      }
    }
  }
  
  override class var layerClass: AnyClass {
    return LoaderLayer.self
  }
  
  private(set) var isAnimating: Bool = false
  
  func start() {
    isAnimating = true
    loaderLayer.add(rotateAnimation, forKey: "rotation")
  }
  
  func stop() {
    isAnimating = false
    loaderLayer.removeAnimation(forKey: "rotation")
  }
  
  let size: Size
  
  init(size: Size) {
    self.size = size
    
    super.init(frame: CGRect(origin: .zero, size: size.size))
    
    layer.addSublayer(loaderLayer)
    (layer as? LoaderLayer)?.parent = self
  }
  
  override var tintColor: UIColor! {
    didSet {
      loaderLayer.strokeColor = tintColor.cgColor
    }
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    
    loaderLayer.frame = bounds
    loaderLayer.anchorPoint = CGPoint(x: 0.5, y: 0.5)
  }
  
  override var intrinsicContentSize: CGSize {
    return size.size
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  // MARK: - Private
  
  private var rotateAnimation = CABasicAnimation(keyPath: "transform.rotation").then {
    $0.repeatCount = .infinity
//    $0.timingFunction = CAMediaTimingFunction(name: .easeIn)
    $0.duration = 0.75
    $0.fromValue = 0
    $0.toValue = CGFloat.pi * 2
  }
  
  private lazy var loaderLayer = CAShapeLayer().then {
    $0.lineCap = .round
    $0.strokeEnd = 0.5
    $0.strokeColor = tintColor.cgColor
    $0.lineWidth = size.lineWidth
    $0.fillColor = nil
    $0.path = UIBezierPath(ovalIn: bounds.insetBy(dx: size.lineWidth / 2.0, dy: size.lineWidth / 2.0)).cgPath
  }
}
