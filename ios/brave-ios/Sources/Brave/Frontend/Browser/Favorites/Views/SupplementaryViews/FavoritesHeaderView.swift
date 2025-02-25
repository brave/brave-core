// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

class FavoritesHeaderView: UICollectionReusableView {
  let label = UILabel().then {
    $0.text = Strings.recentSearchFavorites
    $0.textColor = UIColor(braveSystemName: .textPrimary)

    let desc = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .body)
    $0.font = UIFont.systemFont(
      ofSize: desc.pointSize,
      weight: .semibold
    )
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(label)
    label.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
