// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import SnapKit
import BraveUI

class DetailActionableRow: Button {
  
  let textLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 14.0)
    $0.appearanceTextColor = Colors.grey700
    $0.numberOfLines = 0
  }
  
  let batValueView = CurrencyContainerView(amountLabelConfig: {
    $0.appearanceTextColor = Colors.purple600
    $0.font = .systemFont(ofSize: 14.0, weight: .semibold)
  }, kindLabelConfig: {
    $0.appearanceTextColor = Colors.grey700
    $0.text = Strings.BAT
    $0.font = .systemFont(ofSize: 13.0)
  })
  
  override var isHighlighted: Bool {
    didSet {
      UIView.animate(withDuration: isHighlighted ? 0.05 : 0.4, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
        self.alpha = self.isHighlighted ? 0.3 : 1.0
      }, completion: nil)
    }
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    setImage(UIImage(frameworkResourceNamed: "right-arrow").alwaysTemplate, for: .normal)
    contentHorizontalAlignment = .right
    tintColor = Colors.grey700
    setContentHuggingPriority(.required, for: .horizontal)
    addSubview(textLabel)
    addSubview(batValueView)
    textLabel.snp.makeConstraints {
      $0.leading.top.bottom.equalTo(self)
      $0.trailing.lessThanOrEqualTo(batValueView)
    }
    batValueView.snp.makeConstraints {
      $0.centerY.equalTo(self)
      $0.trailing.equalTo(self).inset(16)
    }
    snp.makeConstraints {
      $0.height.equalTo(40.0)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
