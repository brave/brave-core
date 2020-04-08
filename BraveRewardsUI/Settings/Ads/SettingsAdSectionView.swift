/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class SettingsAdSectionView: SettingsSectionView {
  
  private struct UX {
    static let comingSoonTextColor = UIColor(hex: 0xC9B5DE) // Has to match icon color (which has no close color)
  }
  
  func setSectionEnabled(_ enabled: Bool, hidesToggle: Bool, animated: Bool = false) {
    if !status.isSupported {
      return
    }
    
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
  
  enum SupportStatus {
    case supported
    case unsupportedRegion
    case unsupportedDevice
    
    var isSupported: Bool {
      return self == .supported
    }
    var reason: String? {
      switch self {
      case .supported:
        return nil
      case .unsupportedRegion:
        return Strings.adsUnsupportedRegion
      case .unsupportedDevice:
        return Strings.adsUnsupportedDevice
      }
    }
  }
  
  /// Update the visual appearance based on whether or not Ads are currently supported
  var status: SupportStatus = .supported {
    didSet {
      let isSupported = status.isSupported
      unsupportedView.reason = status.reason
      unsupportedView.isHidden = isSupported
      viewDetailsButton.isHidden = !isSupported
      disabledView.isHidden = !isSupported
      toggleSwitch.isHidden = !isSupported
      
      let alpha: CGFloat = isSupported ? 1.0 : 0.5
      bodyLabel.alpha = alpha
      titleLabel.alpha = alpha
      
      titleLabel.appearanceTextColor = isSupported ? BraveUX.adsTintColor : Colors.grey700
    }
  }
  
  private let unsupportedView = AdsUnsupportedView().then {
    $0.isHidden = true // Default is hidden
  }
  
  let viewDetailsButton = SettingsViewDetailsButton(type: .system)
  private let disabledView = DisabledSettingGraphicView(
    image: UIImage(frameworkResourceNamed: "ads-disabled-icon"),
    text: Strings.disabledAdsMessage
  ).then {
    $0.isHidden = true
  }
  
  let toggleSwitch = UISwitch().then {
    $0.onTintColor = BraveUX.switchOnColor
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    viewDetailsButton.hitTestSlop = UIEdgeInsets(top: -contentStackView.spacing, left: 0, bottom: -contentStackView.spacing, right: 0)
    
    contentStackView.layoutMargins = layoutMargins
    unsupportedView.layoutMargins = layoutMargins
    
    clippedContentView.addSubview(stackView)
    stackView.addArrangedSubview(contentStackView)
    stackView.addArrangedSubview(unsupportedView)
    contentStackView.addArrangedSubview(toggleStackView)
    contentStackView.addArrangedSubview(bodyLabel)
    contentStackView.addArrangedSubview(viewDetailsButton)
    contentStackView.addArrangedSubview(disabledView)
    toggleStackView.addArrangedSubview(titleLabel)
    toggleStackView.addArrangedSubview(toggleSwitch)
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
  }
  
  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 10.0
  }
  
  private let contentStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 10.0
    $0.isLayoutMarginsRelativeArrangement = true
  }
  
  private let toggleStackView = UIStackView().then {
    $0.spacing = 10.0
  }
  
  private let titleLabel = UILabel().then {
    $0.text = Strings.settingsAdsTitle
    $0.appearanceTextColor = BraveUX.adsTintColor
    $0.font = SettingsUX.titleFont
  }

  private let bodyLabel = UILabel().then {
    $0.text = Strings.settingsAdsBody
    $0.appearanceTextColor = SettingsUX.bodyTextColor
    $0.numberOfLines = 0
    $0.font = SettingsUX.bodyFont
  }
}
