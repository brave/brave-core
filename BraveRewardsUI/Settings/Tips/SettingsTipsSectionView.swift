/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class SettingsTipsSectionView: SettingsSectionView {
  
  func setSectionEnabled(_ enabled: Bool, animated: Bool = false) {
    if animated {
      if enabled {
        viewDetailsButton.alpha = 0.0
      } else {
        if disabledView.isHidden {
          disabledView.alpha = 0.0
        }
      }
      UIView.animate(withDuration: 0.15) {
        if self.viewDetailsButton.isHidden == enabled { // UIStackView bug, have to check first
          self.viewDetailsButton.isHidden = !enabled
        }
        if self.disabledView.isHidden != enabled {
          self.disabledView.isHidden = enabled
        }
        self.viewDetailsButton.alpha = enabled ? 1.0 : 0.0
        self.disabledView.alpha = enabled ? 0.0 : 1.0
      }
    } else {
      viewDetailsButton.isHidden = !enabled
      disabledView.isHidden = enabled
    }
  }
  
  let viewDetailsButton = SettingsViewDetailsButton(type: .system)
  private let disabledView = DisabledSettingGraphicView(
    image: UIImage(frameworkResourceNamed: "tips-disabled-icon"),
    text: Strings.disabledTipsMessage
  ).then {
    $0.isHidden = true
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(stackView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(bodyLabel)
    stackView.addArrangedSubview(viewDetailsButton)
    stackView.addArrangedSubview(disabledView)
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self.layoutMarginsGuide)
    }
  }
  
  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 10.0
  }
  
  private let titleLabel = UILabel().then {
    $0.text = Strings.settingsTipsTitle
    $0.appearanceTextColor = BraveUX.tipsTintColor
    $0.font = SettingsUX.titleFont
  }
  
  private let bodyLabel = UILabel().then {
    $0.text = Strings.settingsTipsBody
    $0.appearanceTextColor = SettingsUX.bodyTextColor
    $0.numberOfLines = 0
    $0.font = SettingsUX.bodyFont
  }
}
