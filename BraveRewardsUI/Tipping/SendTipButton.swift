/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class SendTipButton: UIControl {
  
  /// Whether or not the tip is a monthly tip
  var isMonthly: Bool = false {
    didSet {
      textLabel.text = (isMonthly ? Strings.tippingSendMonthlyTip : Strings.tippingSendTip).uppercased()
    }
  }
  
  private let stackView = UIStackView().then {
    $0.spacing = 15.0
    $0.isUserInteractionEnabled = false
  }
  
  private let imageView = UIImageView(image: UIImage(frameworkResourceNamed: "airplane-icn").alwaysTemplate).then {
    $0.tintColor = Colors.blurple300
  }
  
  private let textLabel = UILabel().then {
    $0.appearanceTextColor = .white
    $0.font = .systemFont(ofSize: 13.0, weight: .semibold)
    $0.text = Strings.tippingSendTip.uppercased()
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    backgroundColor = Colors.blurple500
    
    addSubview(stackView)
    stackView.addArrangedSubview(imageView)
    stackView.addArrangedSubview(textLabel)
    
    let contentGuide = UILayoutGuide()
    addLayoutGuide(contentGuide)
    
    contentGuide.snp.makeConstraints {
      $0.top.leading.trailing.equalTo(self)
      $0.bottom.equalTo(self.safeAreaLayoutGuide)
      $0.height.equalTo(56.0)
    }
    stackView.snp.makeConstraints {
      $0.center.equalTo(contentGuide)
      $0.leading.greaterThanOrEqualTo(contentGuide)
      $0.trailing.lessThanOrEqualTo(contentGuide)
    }
  }
  
  // MARK: -
  
  override var isHighlighted: Bool {
    didSet {
      // Replicating usual UIButton highlight animation
      UIView.animate(withDuration: isHighlighted ? 0.05 : 0.4, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
        self.textLabel.alpha = self.isHighlighted ? 0.3 : 1.0
        self.imageView.alpha = self.isHighlighted ? 0.3 : 1.0
      }, completion: nil)
    }
  }
  
  override var isEnabled: Bool {
    didSet {
      self.textLabel.alpha = self.isEnabled ? 1.0 : 0.3
      self.imageView.alpha = self.isEnabled ? 1.0 : 0.3
    }
  }
}
