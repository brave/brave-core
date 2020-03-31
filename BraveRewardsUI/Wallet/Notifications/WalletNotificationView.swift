/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class WalletNotificationView: UIView {
  
  let closeButton = Button()
  
  let backgroundView = UIImageView(image: UIImage(frameworkResourceNamed: "notification_header"))
  let iconImageView = UIImageView(image: nil).then {
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.contentMode = .scaleAspectFit
  }
  
  let stackView = UIStackView()
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    backgroundColor = .clear
    
    closeButton.do {
      $0.setImage(UIImage(frameworkResourceNamed: "close-icon").alwaysTemplate, for: .normal)
      $0.tintColor = Colors.grey600
      $0.contentMode = .center
    }
    
    addSubview(backgroundView)
    addSubview(closeButton)
    addSubview(stackView)
    stackView.addArrangedSubview(iconImageView)
    
    closeButton.snp.makeConstraints {
      $0.top.equalTo(self).inset(PopoverArrowHeight)
      $0.trailing.equalTo(self)
      $0.width.height.equalTo(44.0)
    }
    
    stackView.snp.makeConstraints {
      $0.top.greaterThanOrEqualTo(self).offset(15.0)
      $0.centerY.equalToSuperview()
      $0.leading.trailing.equalTo(safeAreaLayoutGuide).inset(35.0)
      $0.bottom.lessThanOrEqualTo(self).inset(25.0)
    }
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    backgroundView.frame = bounds
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
