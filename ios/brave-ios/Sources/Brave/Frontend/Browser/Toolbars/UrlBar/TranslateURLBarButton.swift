// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import UIKit

class TranslateURLBarButton: UIButton {
  var selectedTintColor: UIColor? {
    didSet {
      updateAppearance()
    }
  }

  var unselectedTintColor: UIColor? {
    didSet {
      updateAppearance()
    }
  }

  var imageIcon: UIImage? {
    UIImage(braveSystemNamed: "leo.product.translate")
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    adjustsImageWhenHighlighted = false
    setImage(imageIcon, for: .normal)
    updateIconSize()
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  override var isSelected: Bool {
    didSet {
      updateAppearance()
    }
  }

  override open var isHighlighted: Bool {
    didSet {
      updateAppearance()
    }
  }

  override var tintColor: UIColor! {
    didSet {
      self.imageView?.tintColor = self.tintColor
    }
  }

  private func updateAppearance() {
    self.tintColor = (isHighlighted || isSelected) ? selectedTintColor : unselectedTintColor
  }

  private var _translateState: TranslateState = .unavailable

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateIconSize()
  }

  private func updateIconSize() {
    let sizeCategory = traitCollection.toolbarButtonContentSizeCategory
    let pointSize = UIFont.preferredFont(
      forTextStyle: .body,
      compatibleWith: .init(preferredContentSizeCategory: sizeCategory)
    ).pointSize
    setPreferredSymbolConfiguration(
      .init(pointSize: pointSize, weight: .regular, scale: .medium),
      forImageIn: .normal
    )
  }

  var translateState: TranslateState {
    get {
      return _translateState
    }
    set(state) {
      _translateState = state
      switch _translateState {
      case .available:
        self.isEnabled = true
        self.isSelected = false
      case .unavailable:
        self.isEnabled = false
        self.isSelected = false
      case .active:
        self.isEnabled = true
        self.isSelected = true
      }
    }
  }

  private lazy var gradientLayer = CAGradientLayer().then {
    let gradient = BraveGradient(
      stops: [
        .init(color: UIColor(rgb: 0xFA7250), position: 0.0),
        .init(color: UIColor(rgb: 0xFF1893), position: 0.43),
        .init(color: UIColor(rgb: 0xA78AFF), position: 1.0),
      ],
      angle: .figmaDegrees(314.42)
    )

    $0.frame = self.bounds
    $0.type = gradient.type
    $0.colors = gradient.stops.map(\.color.cgColor)
    $0.locations = gradient.stops.map({ NSNumber(value: $0.position) })
    $0.startPoint = gradient.startPoint
    $0.endPoint = gradient.endPoint

    let mask = CALayer()
    mask.contents = imageIcon?.cgImage
    mask.frame = $0.bounds
    $0.mask = mask
  }

  func setOnboardingState(enabled: Bool) {
    if enabled {
      gradientLayer.frame = imageView?.bounds ?? self.bounds
      gradientLayer.mask?.frame = gradientLayer.bounds

      imageView?.layer.addSublayer(gradientLayer)
      setImage(nil, for: .normal)
    } else {
      gradientLayer.removeFromSuperlayer()
      setImage(imageIcon, for: .normal)
    }
  }

  enum TranslateState {
    case available
    case unavailable
    case active
  }
}
