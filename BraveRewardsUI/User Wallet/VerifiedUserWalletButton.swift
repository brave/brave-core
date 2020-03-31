// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveShared
import pop
import BraveUI

class VerifiedUserWalletButton: UIControl {
  
  private let stackView = UIStackView().then {
    $0.spacing = 8
    $0.alignment = .center
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(stackView)
    
    stackView.isUserInteractionEnabled = false
    stackView.addStackViewItems(
      .view(UIImageView(image: UIImage(frameworkResourceNamed: "uphold").alwaysTemplate).then {
        $0.tintColor = BraveUX.upholdGreen
        $0.snp.makeConstraints {
          $0.width.equalTo(12)
          $0.height.equalTo(15)
        }
      }),
      .view(UILabel().then {
        $0.text = Strings.userWalletVerifiedButtonTitle
        $0.font = .systemFont(ofSize: 13, weight: .regular)
        $0.appearanceTextColor = .white
      }),
      .view(UIImageView(image: UIImage(frameworkResourceNamed: "verified-arrow")))
    )
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override var isHighlighted: Bool {
    didSet {
      stackView.basicAnimate(property: kPOPViewAlpha, key: "alpha") { animation, _ in
        animation.toValue = self.isHighlighted ? 0.5 : 1.0
        animation.duration = 0.1
      }
    }
  }
  
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    if bounds.inset(by: UIEdgeInsets(top: -12, left: -8, bottom: -12, right: -8)).contains(point) {
      return true
    }
    return super.point(inside: point, with: event)
  }
}
