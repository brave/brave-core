/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class SettingsRewardsSectionView: SettingsSectionView {
  
  var toggleSwitch: UISwitch {
    return toggleView.toggleSwitch
  }
  
  /// Set the rewards enabled state based on the ledger.
  func setRewardsEnabled(_ enabled: Bool, animated: Bool = false) {
    if disabledTextView.isHidden == enabled {
      // Nothing to do
      return
    }
    if animated {
      if !enabled {
        self.disabledTextView.alpha = 0
      }
      UIView.animate(withDuration: 0.25) {
        self.disabledTextView.isHidden = enabled
        self.disabledTextView.alpha = enabled ? 0.0 : 1.0
      }
    } else {
      disabledTextView.isHidden = enabled
    }
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(stackView)
    stackView.addArrangedSubview(toggleView)
    stackView.addArrangedSubview(disabledTextView)
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self).inset(SettingsUX.layoutMargins)
    }
  }
  
  // MARK: - Private UI
  
  private let toggleView = BraveRewardsToggleView()
  
  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 15.0
  }
  
  private let disabledTextView = DisabledRewardsLabelsView()
}

private class DisabledRewardsLabelsView: UIView {
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(stackView)
    labels.forEach { stackView.addArrangedSubview($0) }
    stackView.setCustomSpacing(16.0, after: labels[1])
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 8.0
  }
  
  let labels = [
    UILabel().then {
      $0.textColor = SettingsUX.headerTextColor
      $0.text = Strings.SettingsDisabledTitle1
      $0.numberOfLines = 0
      $0.font = .systemFont(ofSize: 15.0)
    },
    UILabel().then {
      $0.textColor = SettingsUX.bodyTextColor
      $0.text = Strings.SettingsDisabledBody1
      $0.numberOfLines = 0
      $0.font = .systemFont(ofSize: 13.0)
    },
    UILabel().then {
      $0.textColor = SettingsUX.headerTextColor
      $0.text = Strings.SettingsDisabledTitle2
      $0.numberOfLines = 0
      $0.font = .systemFont(ofSize: 15.0)
    },
    UILabel().then {
      $0.textColor = SettingsUX.bodyTextColor
      $0.text = Strings.SettingsDisabledBody2
      $0.numberOfLines = 0
      $0.font = .systemFont(ofSize: 13.0)
    },
  ]
}
