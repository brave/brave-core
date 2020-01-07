// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import pop

/// The button to place on the wallet header when the user hasn't currently
/// connected with Uphold
class VerifyUserWalletButton: UIControl {
  
  private let backgroundView = UIView().then {
    $0.appearanceBackgroundColor = UIColor(hex: 0x4c54d2)
  }
  
  private let stackView = UIStackView().then {
    $0.spacing = 8
    $0.alignment = .center
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(backgroundView)
    addSubview(stackView)
    
    backgroundView.isUserInteractionEnabled = false
    stackView.isUserInteractionEnabled = false
    stackView.addStackViewItems(
      .view(UILabel().then {
        $0.text = Strings.UserWalletUnverifiedButtonTitle
        $0.font = .systemFont(ofSize: 12, weight: .semibold)
        $0.appearanceTextColor = .white
      }),
      .view(UIImageView(image: UIImage(frameworkResourceNamed: "verified-arrow")))
    )
    
    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self).inset(UIEdgeInsets(top: 6, left: 12, bottom: 6, right: 12))
    }
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    
    backgroundView.layer.cornerRadius = bounds.height / 2.0
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override var isHighlighted: Bool {
    didSet {
      basicAnimate(property: kPOPViewAlpha, key: "alpha") { animation, _ in
        animation.toValue = self.isHighlighted ? 0.5 : 1.0
        animation.duration = 0.1
      }
    }
  }
  
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    if bounds.inset(by: UIEdgeInsets(top: -8, left: -8, bottom: -8, right: -8)).contains(point) {
      return true
    }
    return super.point(inside: point, with: event)
  }
}
