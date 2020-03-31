// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

/// A small disclosure that appears in the SettingsAdsSectionView if ads are not supported
/// on this device for some given reason
class AdsUnsupportedView: UIView {
  
  /// The reason to display in the box
  var reason: String? {
    didSet {
      textLabel.text = reason
    }
  }
  
  private let textLabel = UILabel().then {
    $0.numberOfLines = 0
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
    $0.font = .systemFont(ofSize: 13.0, weight: .medium)
    $0.appearanceTextColor = Colors.neutral700
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    backgroundColor = Colors.blurple100
    
    let stackView = UIStackView().then {
      $0.spacing = 15.0
      $0.alignment = .center
    }
    
    let imageView = UIImageView(image: UIImage(frameworkResourceNamed: "info-large")).then {
      $0.setContentHuggingPriority(.required, for: .horizontal)
    }

    addSubview(stackView)
    stackView.addArrangedSubview(imageView)
    stackView.addArrangedSubview(textLabel)
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(layoutMarginsGuide)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
