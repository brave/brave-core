/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class SettingsAutoContributeSectionView: SettingsSectionView {
  
  func setSectionEnabled(_ enabled: Bool, hidesToggle: Bool, animated: Bool = false) {
    if animated {
      if enabled {
        if viewDetailsButton.isHidden {
          viewDetailsButton.alpha = 0.0
        }
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
        self.toggleSwitch.alpha = hidesToggle ? 0.0 : 1.0
      }
    } else {
      viewDetailsButton.isHidden = !enabled
      disabledView.isHidden = enabled
      self.toggleSwitch.alpha = hidesToggle ? 0.0 : 1.0
    }
  }
  
  let toggleSwitch = UISwitch().then {
    $0.onTintColor = BraveUX.switchOnColor
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }
  
  let viewDetailsButton = SettingsViewDetailsButton(type: .system)
  private let disabledView = DisabledSettingGraphicView(
    image: UIImage(frameworkResourceNamed: "ac-disabled-icon"),
    text: Strings.disabledAutoContributeMessage
  ).then {
    $0.isHidden = true
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    viewDetailsButton.hitTestSlop = UIEdgeInsets(top: -stackView.spacing, left: 0, bottom: -stackView.spacing, right: 0)
    
    addSubview(stackView)
    stackView.addArrangedSubview(toggleStackView)
    stackView.addArrangedSubview(bodyLabel)
    stackView.addArrangedSubview(viewDetailsButton)
    stackView.addArrangedSubview(disabledView)
    toggleStackView.addArrangedSubview(titleLabel)
    toggleStackView.addArrangedSubview(toggleSwitch)
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self.layoutMarginsGuide)
    }
  }
  
  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 10.0
  }
  
  private let toggleStackView = UIStackView().then {
    $0.spacing = 10.0
  }
  
  private let titleLabel = UILabel().then {
    $0.text = Strings.settingsAutoContributeTitle
    $0.appearanceTextColor = BraveUX.autoContributeTintColor
    $0.font = SettingsUX.titleFont
  }
  
  private let bodyLabel = UILabel().then {
    $0.text = Strings.settingsAutoContributeBody
    $0.appearanceTextColor = SettingsUX.bodyTextColor
    $0.numberOfLines = 0
    $0.font = SettingsUX.bodyFont
  }
}
