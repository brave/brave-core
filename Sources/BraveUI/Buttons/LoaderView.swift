/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SwiftUI

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
public class LoaderView: UIView {
  /// The size of the indicator
  public enum Size {
    case mini
    case small
    case normal
    case large
    
    public var width: CGFloat { return size.width }
    public var height: CGFloat { return size.height }

    fileprivate var size: CGSize {
      switch self {
      case .mini:
        return CGSize(width: 8, height: 8)
      case .small:
        return CGSize(width: 16, height: 16)
      case .normal:
        return CGSize(width: 32, height: 32)
      case .large:
        return CGSize(width: 64, height: 64)
      }
    }

    fileprivate var lineWidth: CGFloat {
      switch self {
      case .mini:
        return 1.0
      case .small:
        return 2.0
      case .normal:
        return 3.0
      case .large:
        return 4.0
      }
    }
  }

  override public class var layerClass: AnyClass {
    return LoaderLayer.self
  }

  private(set) public var isAnimating: Bool = false

  public func start() {
    isAnimating = true
    loaderLayer.add(rotateAnimation, forKey: "rotation")
  }

  public func stop() {
    isAnimating = false
    loaderLayer.removeAnimation(forKey: "rotation")
  }

  public let size: Size

  public init(size: Size) {
    self.size = size

    super.init(frame: CGRect(origin: .zero, size: size.size))

    layer.addSublayer(loaderLayer)
    (layer as? LoaderLayer)?.parent = self
  }

  override public var tintColor: UIColor! {
    didSet {
      loaderLayer.strokeColor = tintColor.resolvedColor(with: traitCollection).cgColor
    }
  }

  override public func layoutSubviews() {
    super.layoutSubviews()

    loaderLayer.frame = bounds
    loaderLayer.anchorPoint = CGPoint(x: 0.5, y: 0.5)
  }

  override public var intrinsicContentSize: CGSize {
    return size.size
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  // MARK: - Private

  private var rotateAnimation: CABasicAnimation = {
    let anim = CABasicAnimation(keyPath: "transform.rotation")
    anim.repeatCount = .infinity
    //    $0.timingFunction = CAMediaTimingFunction(name: .easeIn)
    anim.duration = 0.75
    anim.fromValue = 0
    anim.toValue = CGFloat.pi * 2
    return anim
  }()

  private lazy var loaderLayer: CAShapeLayer = {
    let layer = CAShapeLayer()
    layer.lineCap = .round
    layer.strokeEnd = 0.5
    layer.strokeColor = tintColor.resolvedColor(with: traitCollection).cgColor
    layer.lineWidth = size.lineWidth
    layer.fillColor = nil
    layer.path = UIBezierPath(ovalIn: bounds.insetBy(dx: size.lineWidth / 2.0, dy: size.lineWidth / 2.0)).cgPath
    return layer
  }()
}

public struct BraveCircularProgressViewStyle: ProgressViewStyle {
  public var size: LoaderView.Size = .normal
  public var tintColor: UIColor = .white
  @State private var isActive: Bool = false
  
  public func makeBody(configuration: Configuration) -> some View {
    Representable(size: size, tintColor: tintColor, isActive: $isActive)
      .onAppear {
        isActive = true
      }
      .onDisappear {
        isActive = false
      }
  }
  struct Representable: UIViewRepresentable {
    var size: LoaderView.Size
    var tintColor: UIColor
    @Binding var isActive: Bool
    
    func makeUIView(context: Context) -> LoaderView {
      LoaderView(size: size)
    }
    func updateUIView(_ uiView: LoaderView, context: Context) {
      uiView.setContentHuggingPriority(.required, for: .vertical)
      uiView.setContentHuggingPriority(.required, for: .horizontal)
      uiView.tintColor = tintColor
      if isActive {
        uiView.start()
      } else {
        uiView.stop()
      }
    }
  }
}

extension ProgressViewStyle where Self == BraveCircularProgressViewStyle {
  public static func braveCircular(
    size: LoaderView.Size = .normal,
    tint: UIColor = .white
  ) -> BraveCircularProgressViewStyle {
    .init(size: size, tintColor: tint)
  }
}
