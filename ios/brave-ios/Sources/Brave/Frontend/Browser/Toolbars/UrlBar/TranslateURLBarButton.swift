// Copyright 2021 The Brave Authors. All rights reserved.
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

  override init(frame: CGRect) {
    super.init(frame: frame)
    adjustsImageWhenHighlighted = false
    setImage(UIImage(braveSystemNamed: "leo.product.translate"), for: .normal)
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

  enum TranslateState {
    case available
    case unavailable
    case active
  }
}
