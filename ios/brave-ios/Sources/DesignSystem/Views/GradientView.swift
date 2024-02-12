// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import SwiftUI

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
    return layer as! CAGradientLayer  // swiftlint:disable:this force_cast
  }

  public override class var layerClass: AnyClass {
    return CAGradientLayer.self
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }
}

/// A gradient view which uses a BraveGradient adjusts its gradient properties based on a
/// trait collection.
///
/// If you want to resolve a `BraveGradient` to a specific user interface style, use
/// `init(gradient:)` or set `overrideUserInterfaceStyle` if you have used one of the pre-set
/// providers
public class BraveGradientView: GradientView {
  private var provider: (UITraitCollection) -> BraveGradient

  public init(dynamicProvider provider: @escaping (UITraitCollection) -> BraveGradient) {
    self.provider = provider
    super.init(colors: [], positions: [], startPoint: .zero, endPoint: .zero)
    updateGradient()
  }

  public convenience init(gradient: BraveGradient) {
    self.init(dynamicProvider: { _ in gradient })
  }

  private func updateGradient() {
    let gradient = provider(traitCollection)
    gradientLayer.type = gradient.type
    gradientLayer.colors = gradient.stops.map(\.color.cgColor)
    gradientLayer.locations = gradient.stops.map({ NSNumber(value: $0.position) })
    gradientLayer.startPoint = gradient.startPoint
    gradientLayer.endPoint = gradient.endPoint
  }

  public override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if traitCollection.hasDifferentColorAppearance(comparedTo: previousTraitCollection) {
      updateGradient()
    }
  }
}

/// A gradient control which uses a BraveGradient adjusts its gradient properties based on a
/// trait collection.
public class BraveGradientButton: UIButton {
  private var provider: (UITraitCollection) -> BraveGradient

  public init(dynamicProvider provider: @escaping (UITraitCollection) -> BraveGradient, colors: [UIColor], positions: [CGFloat], startPoint: CGPoint, endPoint: CGPoint) {
    self.provider = provider
    
    super.init(frame: .zero)

    gradientLayer.colors = colors.map { $0.resolvedColor(with: traitCollection).cgColor }
    gradientLayer.locations = positions.map { NSNumber(value: Double($0)) }
    gradientLayer.startPoint = startPoint
    gradientLayer.endPoint = endPoint
    
    updateGradient()
  }
  
  public convenience init(gradient: BraveGradient) {
    self.init(dynamicProvider: { _ in gradient }, colors: [], positions: [], startPoint: .zero, endPoint: CGPoint(x: 0, y: 1))
  }
  
  /// The gradient layer which you may modify
  public var gradientLayer: CAGradientLayer {
    return layer as! CAGradientLayer  // swiftlint:disable:this force_cast
  }

  public override class var layerClass: AnyClass {
    return CAGradientLayer.self
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  private func updateGradient() {
    let gradient = provider(traitCollection)
    gradientLayer.type = gradient.type
    gradientLayer.colors = gradient.stops.map(\.color.cgColor)
    gradientLayer.locations = gradient.stops.map({ NSNumber(value: $0.position) })
    gradientLayer.startPoint = gradient.startPoint
    gradientLayer.endPoint = gradient.endPoint
  }

  public override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if traitCollection.hasDifferentColorAppearance(comparedTo: previousTraitCollection) {
      updateGradient()
    }
  }
}
