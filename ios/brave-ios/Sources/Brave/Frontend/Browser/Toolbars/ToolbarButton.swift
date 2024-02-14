// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

extension UITraitCollection {
  /// Returns the size category to be used for toolbar buttons based on the current preferred size category
  var toolbarButtonContentSizeCategory: UIContentSizeCategory {
    let sizeCategory = preferredContentSizeCategory
    if sizeCategory < UIContentSizeCategory.extraLarge {
      return .large
    } else if sizeCategory < UIContentSizeCategory.extraExtraLarge {
      return .extraLarge
    }
    return .extraExtraLarge
  }
}

class ToolbarButton: UIButton {
  var selectedTintColor: UIColor? {
    didSet {
      updateTintColor()
    }
  }
  var primaryTintColor: UIColor? {
    didSet {
      updateTintColor()
    }
  }
  var disabledTintColor: UIColor? {
    didSet {
      updateTintColor()
    }
  }

  init() {
    super.init(frame: .zero)
    adjustsImageWhenHighlighted = false
    imageView?.contentMode = .scaleAspectFit
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override open var isHighlighted: Bool {
    didSet {
      updateTintColor()
    }
  }

  override open var isEnabled: Bool {
    didSet {
      updateTintColor()
    }
  }

  override var tintColor: UIColor! {
    didSet {
      self.imageView?.tintColor = self.tintColor
    }
  }
  
  private func updateTintColor() {
    let tintColor: UIColor? = {
      if !isEnabled {
        if let disabledTintColor {
          return disabledTintColor
        } else {
          return primaryTintColor?.withAlphaComponent(0.4)
        }
      }
      if isHighlighted {
        return selectedTintColor
      }
      return primaryTintColor
    }()
    self.tintColor = tintColor
  }

  override func contextMenuInteraction(_ interaction: UIContextMenuInteraction, willDisplayMenuFor configuration: UIContextMenuConfiguration, animator: UIContextMenuInteractionAnimating?) {
    UIImpactFeedbackGenerator(style: .medium).bzzt()
  }
}
