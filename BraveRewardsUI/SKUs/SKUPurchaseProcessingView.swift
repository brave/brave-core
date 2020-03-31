// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

/// The loading view
class SKUPurchaseProcessingView: UIView {
  
  let loaderView = LoaderView(size: .large).then {
    $0.tintColor = Colors.blurple500
    $0.setContentHuggingPriority(.required, for: .vertical)
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 28
      $0.alignment = .center
    }
    
    addSubview(stackView)
    stackView.snp.makeConstraints {
      $0.center.equalTo(self)
      $0.top.leading.greaterThanOrEqualTo(self).inset(30)
      $0.bottom.trailing.lessThanOrEqualTo(self).inset(30)
    }
    
    stackView.addArrangedSubview(loaderView)
    stackView.addArrangedSubview(UILabel().then {
      $0.text = Strings.SKUPurchaseProcessing
      $0.textAlignment = .center
      $0.numberOfLines = 0
      $0.appearanceTextColor = Colors.grey900
      $0.font = .systemFont(ofSize: 15)
    })
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
