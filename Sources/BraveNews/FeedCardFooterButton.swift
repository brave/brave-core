// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

class FeedCardFooterButton: UIControl {
  let label = UILabel().then {
    $0.textColor = .white
    $0.font = .systemFont(ofSize: 14, weight: .semibold)
    $0.isAccessibilityElement = false
  }
  private let disclosureIcon = UIImageView(image: UIImage(named: "disclosure-arrow", in: .module, compatibleWith: nil)!.template).then {
    $0.tintColor = .white
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    let stackView = UIStackView(arrangedSubviews: [label, disclosureIcon])
    stackView.alignment = .center
    stackView.isUserInteractionEnabled = false
    addSubview(stackView)
    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(UIEdgeInsets(top: 0, left: 20, bottom: 0, right: 20))
    }
    snp.makeConstraints {
      $0.height.equalTo(44)
    }
    accessibilityTraits.insert(.button)
    isAccessibilityElement = true
  }

  override var accessibilityLabel: String? {
    get { label.text }
    set { assertionFailure("Accessibility label is inherited from a subview: \(String(describing: newValue)) ignored") }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override var isHighlighted: Bool {
    didSet {
      UIViewPropertyAnimator(duration: 0.3, dampingRatio: 1.0) {
        self.backgroundColor = self.isHighlighted ? UIColor(white: 1.0, alpha: 0.1) : UIColor.clear
      }
      .startAnimation()
    }
  }
}
