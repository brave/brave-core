/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class SettingsTipsSectionView: SettingsSectionView {
  
  func setSectionEnabled(_ enabled: Bool, animated: Bool = false) {
    if viewDetailsButton.isHidden != enabled {
      // Nothing to do
      return
    }
    
    if animated {
      if enabled {
        viewDetailsButton.alpha = 0.0
      }
      UIView.animate(withDuration: 0.15) {
        self.viewDetailsButton.isHidden = !enabled
        self.viewDetailsButton.alpha = enabled ? 1.0 : 0.0
      }
    } else {
      viewDetailsButton.isHidden = !enabled
    }
  }
  
  let viewDetailsButton = SettingsViewDetailsButton(type: .system)
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(stackView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(bodyLabel)
    stackView.addArrangedSubview(viewDetailsButton)
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self.layoutMarginsGuide)
    }
  }
  
  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 10.0
  }
  
  private let titleLabel = UILabel().then {
    $0.text = Strings.SettingsTipsTitle
    $0.textColor = BraveUX.tipsTintColor
    $0.font = SettingsUX.titleFont
  }
  
  private let bodyLabel = UILabel().then {
    $0.text = Strings.SettingsTipsBody
    $0.textColor = SettingsUX.bodyTextColor
    $0.numberOfLines = 0
    $0.font = SettingsUX.bodyFont
  }
}
