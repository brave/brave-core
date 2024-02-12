// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveUI
import Shared

class RecentSearchClipboardHeaderView: UICollectionReusableView {
  let button = UIButton().then {
    let image = UIImage(named: "recent-search-link", in: .module, compatibleWith: nil)!
    let paddingX: CGFloat = 15.0
    let paddingY: CGFloat = 10.0
    $0.setImage(image, for: .normal)
    $0.imageEdgeInsets = UIEdgeInsets(top: 0.0, left: 0.0, bottom: 0.0, right: image.size.width / 2.0)
    $0.titleEdgeInsets = UIEdgeInsets(top: -paddingY, left: paddingX, bottom: -paddingY, right: -paddingX * 2.0)
    $0.contentEdgeInsets = UIEdgeInsets(top: paddingY, left: paddingX, bottom: paddingY, right: paddingX * 2.0)
    $0.contentHorizontalAlignment = .left
    $0.imageView?.contentMode = .scaleAspectFit
    $0.setTitle(Strings.recentSearchPasteAndGo, for: .normal)
    $0.setTitleColor(.primaryButtonTint, for: .normal)
    $0.titleLabel?.font = .systemFont(ofSize: 13.0, weight: .semibold)
    $0.layer.borderColor = UIColor.secondaryButtonTint.cgColor
    $0.layer.borderWidth = 1.0
    $0.layer.cornerCurve = .continuous
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(button)
    button.snp.makeConstraints {
      $0.leading.equalToSuperview()
      $0.trailing.lessThanOrEqualToSuperview()
      $0.top.bottom.equalToSuperview()
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    button.do {
      $0.layer.cornerRadius = $0.bounds.height / 2.0
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    traitCollection.performAsCurrent {
      button.layer.borderColor = UIColor.secondaryButtonTint.cgColor
    }
  }
}
