// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SnapKit

class HeaderContainerView: UIView {
  
  let expandedBarStackView = UIStackView().then {
    $0.axis = .vertical
    $0.clipsToBounds = true
  }
  let collapsedBarContainerView = UIControl().then {
    $0.alpha = 0
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(expandedBarStackView)
    addSubview(collapsedBarContainerView)
    
    collapsedBarContainerView.snp.makeConstraints {
      $0.leading.trailing.equalTo(safeAreaLayoutGuide)
      $0.bottom.equalToSuperview()
    }
    expandedBarStackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
