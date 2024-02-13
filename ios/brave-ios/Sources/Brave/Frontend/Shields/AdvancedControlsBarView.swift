// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

class AdvancedControlsBarView: UIControl {

  var isShowingAdvancedControls: Bool = false {
    didSet {
      imageView.transform = CGAffineTransform(rotationAngle: isShowingAdvancedControls ? CGFloat.pi : 0)
    }
  }

  private let topBorderView = UIView().then {
    $0.backgroundColor = .braveSeparator
  }

  private let label = UILabel().then {
    $0.text = Strings.Shields.advancedControls.capitalized
    $0.font = .systemFont(ofSize: 16)
    $0.textColor = .braveLabel
    $0.numberOfLines = 0
  }

  private let imageView = UIImageView().then {
    $0.image = UIImage(named: "advanced-bar-chevron", in: .module, compatibleWith: nil)?
      .withRenderingMode(.alwaysOriginal)
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    let stackView = UIStackView().then {
      $0.alignment = .center
      $0.spacing = 8
      $0.isUserInteractionEnabled = false
    }

    backgroundColor = .secondaryBraveBackground

    isAccessibilityElement = true
    accessibilityTraits.insert(.button)

    addSubview(topBorderView)
    addSubview(stackView)

    stackView.addArrangedSubview(label)
    stackView.addArrangedSubview(imageView)

    accessibilityLabel = label.text
    accessibilityValue = isShowingAdvancedControls ? "1" : "0"

    topBorderView.snp.makeConstraints {
      $0.top.leading.trailing.equalTo(self)
      $0.height.equalTo(1.0 / UIScreen.main.scale)
    }
    stackView.snp.makeConstraints {
      $0.leading.equalTo(self).inset(16)
      $0.top.bottom.equalTo(self).inset(10)
      $0.trailing.equalTo(self).inset(16)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override var isHighlighted: Bool {
    didSet {
      UIView.animate(withDuration: 0.15) {
        self.label.alpha = self.isHighlighted ? 0.4 : 1.0
        self.imageView.alpha = self.isHighlighted ? 0.4 : 1.0
      }
    }
  }

  override func endTracking(_ touch: UITouch?, with event: UIEvent?) {
    if isTouchInside {
      UIView.animate(
        withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.7, initialSpringVelocity: 0, options: [.beginFromCurrentState],
        animations: {
          self.isShowingAdvancedControls.toggle()
        }, completion: nil)
    }
    super.endTracking(touch, with: event)
  }
}
