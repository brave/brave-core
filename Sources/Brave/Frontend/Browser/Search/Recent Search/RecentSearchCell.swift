// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SnapKit
import BraveUI

class RecentSearchCell: UICollectionViewCell, CollectionViewReusable {
  static let identifier = "RecentSearchCell"
  var openButtonAction: (() -> Void)?

  private let stackView = UIStackView().then {
    $0.spacing = 20.0
    $0.isLayoutMarginsRelativeArrangement = true
    $0.layoutMargins = UIEdgeInsets(top: 0.0, left: 12.0, bottom: 0.0, right: 12.0)
  }

  private let titleLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 15.0)
    $0.lineBreakMode = .byTruncatingMiddle
  }

  private let openButton = BraveButton(type: .system).then {
    $0.setImage(UIImage(named: "recent-search-arrow", in: .module, compatibleWith: nil)!, for: .normal)
    $0.imageView?.contentMode = .scaleAspectFit
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.hitTestSlop = UIEdgeInsets(equalInset: -25.0)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    openButton.addTarget(self, action: #selector(onOpenButtonPressed(_:)), for: .touchUpInside)

    contentView.addSubview(stackView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(openButton)

    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func setTitle(_ title: String?) {
    titleLabel.text = title
  }

  func setAttributedTitle(_ title: NSAttributedString?) {
    titleLabel.attributedText = title
  }

  @objc
  private func onOpenButtonPressed(_ button: UIButton) {
    openButtonAction?()
  }
}
