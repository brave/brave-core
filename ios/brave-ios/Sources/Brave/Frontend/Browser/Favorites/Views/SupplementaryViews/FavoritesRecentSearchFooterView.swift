// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

class FavoritesRecentSearchFooterView: UICollectionReusableView {
  let label = UILabel().then {
    $0.text = Strings.recentShowMore
    $0.font = .preferredFont(for: .subheadline, weight: .semibold)
    $0.textColor = UIColor(braveSystemName: .textInteractive)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(label)
    label.snp.makeConstraints {
      $0.top.equalToSuperview()
      $0.leading.trailing.equalToSuperview().inset(12.0)
      $0.bottom.equalToSuperview().inset(24.0)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
