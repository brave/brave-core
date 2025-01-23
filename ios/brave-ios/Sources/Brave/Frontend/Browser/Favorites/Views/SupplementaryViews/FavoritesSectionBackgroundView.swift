// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

class FavoritesSectionBackgroundView: UICollectionReusableView {

  let sectionGroupBackground = UIView()

  override init(frame: CGRect) {
    super.init(frame: frame)

    self.do {
      $0.backgroundColor = UIColor(braveSystemName: .containerBackground).withAlphaComponent(0.55)
      $0.layer.cornerRadius = 16
    }

    addSubview(sectionGroupBackground)
    sectionGroupBackground.snp.makeConstraints {
      $0.top.equalToSuperview().inset(50)  // header height
      $0.leading.trailing.equalToSuperview().inset(4)
      $0.bottom.equalToSuperview().inset(4)
    }

    sectionGroupBackground.do {
      $0.backgroundColor = UIColor(braveSystemName: .containerBackground)
      $0.layer.cornerRadius = 12
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
