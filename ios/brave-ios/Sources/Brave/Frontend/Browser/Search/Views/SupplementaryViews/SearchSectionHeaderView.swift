// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

class SearchSectionHeaderView: UICollectionReusableView, CollectionViewReusable {
  private let label = UILabel()

  override init(frame: CGRect) {
    super.init(frame: frame)

    setTheme()

    addSubview(label)
    label.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    setTheme()
  }

  private func setTheme() {
    label.do {
      $0.textColor = UIColor(braveSystemName: .textPrimary)

      var sizeCategory = UIApplication.shared.preferredContentSizeCategory
      if sizeCategory.isAccessibilityCategory {
        sizeCategory = .large
      }
      let traitCollection = UITraitCollection(preferredContentSizeCategory: sizeCategory)
      let font = UIFont.preferredFont(
        for: .body,
        weight: .semibold,
        traitCollection: traitCollection
      )
      $0.font = font
    }
  }

  func setTitle(_ title: String) {
    label.text = title
  }
}
