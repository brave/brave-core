// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

class SearchSuggestionCell: UICollectionViewCell, CollectionViewReusable {
  var openButtonActionHandler: (() -> Void)?

  private let stackView = UIStackView().then {
    $0.spacing = 16.0
    $0.alignment = .center
  }

  private let searchImageView = UIImageView().then {
    $0.image = UIImage(braveSystemNamed: "leo.search")!.template
    $0.contentMode = .scaleAspectFit
    $0.tintColor = UIColor(braveSystemName: .iconDefault)
  }

  private let titleLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 15.0)
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.lineBreakMode = .byTruncatingMiddle
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  private let openButton = BraveButton().then {
    $0.setImage(
      UIImage(braveSystemNamed: "leo.arrow.diagonal-up-left"),
      for: .normal
    )
    $0.hitTestSlop = UIEdgeInsets(equalInset: -20)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .clear

    contentView.addSubview(stackView)
    stackView.addArrangedSubview(searchImageView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(openButton)

    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    searchImageView.snp.makeConstraints {
      $0.width.height.equalTo(20.0)
    }

    openButton.snp.makeConstraints {
      $0.width.height.equalTo(20.0)
    }

    openButton.addTarget(self, action: #selector(onOpenButtonPressed), for: .touchUpInside)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func setTitle(_ title: String) {
    titleLabel.text = title
  }

  @objc
  private func onOpenButtonPressed() {
    openButtonActionHandler?()
  }
}
