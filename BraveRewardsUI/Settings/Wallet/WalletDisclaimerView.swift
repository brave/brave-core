// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

class WalletDisclaimerView: UIView {
  
  var labels: [LinkLabel] = [] {
    willSet {
      labels.forEach { $0.removeFromSuperview() }
    }
    didSet {
      labels.forEach {
        stackView.addArrangedSubview($0)
      }
    }
  }
  
  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 10
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
  
    addSubview(stackView)
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self).inset(8)
    }
    
    backgroundColor = UIColor(white: 0.0, alpha: 0.04)
    layer.cornerRadius = 4.0
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
