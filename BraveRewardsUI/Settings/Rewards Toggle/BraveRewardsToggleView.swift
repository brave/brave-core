/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class BraveRewardsToggleView: UIView {
  
  let toggleSwitch = UISwitch().then {
    $0.onTintColor = BraveUX.switchOnColor
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(stackView)
    stackView.addArrangedSubview(batLogoImageView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(toggleSwitch)
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  // MARK: - Private UI
  
  private let stackView = UIStackView().then {
    $0.spacing = 10.0
    $0.alignment = .center
  }
  
  private let batLogoImageView = UIImageView(image: UIImage(frameworkResourceNamed: "bat-small")).then {
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }
  
  private let titleLabel = UILabel().then {
    $0.appearanceTextColor = Colors.grey800
    $0.text = Strings.braveRewards
  }
  
  // MARK: - Accessibility
  
  override func accessibilityActivate() -> Bool {
    toggleSwitch.setOn(!toggleSwitch.isOn, animated: true)
    toggleSwitch.sendActions(for: .valueChanged)
    return true
  }
}
