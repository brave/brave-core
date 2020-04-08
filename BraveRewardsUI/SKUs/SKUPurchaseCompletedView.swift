// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

/// The loading view
class SKUPurchaseCompletedView: UIView {
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 18
      $0.alignment = .center
    }
    
    addSubview(stackView)
    stackView.snp.makeConstraints {
      $0.center.equalTo(self)
      $0.top.leading.greaterThanOrEqualTo(self).inset(30)
      $0.bottom.trailing.lessThanOrEqualTo(self).inset(30)
    }
    
    stackView.addStackViewItems(
      .view(UIImageView(image: UIImage(frameworkResourceNamed: "bat_refer")).then {
        $0.setContentHuggingPriority(.required, for: .vertical)
        $0.setContentHuggingPriority(.required, for: .horizontal)
      }),
      .view(UILabel().then {
        $0.text = Strings.SKUPurchaseCompletedTitle
        $0.appearanceTextColor = Colors.grey900
        $0.textAlignment = .center
        $0.numberOfLines = 0
        $0.font = .systemFont(ofSize: 18, weight: .semibold)
      }),
      .customSpace(4),
      .view(UILabel().then {
        $0.text = Strings.SKUPurchaseCompletedBody
        $0.appearanceTextColor = Colors.grey900
        $0.textAlignment = .center
        $0.numberOfLines = 0
        $0.font = .systemFont(ofSize: 14)
      })
    )
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
