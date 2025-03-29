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
    $0.isLayoutMarginsRelativeArrangement = true
    $0.insetsLayoutMarginsFromSafeArea = false
    $0.layoutMargins = .init(vertical: 0, horizontal: 16)
  }

  private let searchImageView = UIImageView().then {
    $0.image = UIImage(braveSystemNamed: "leo.search")!.template
    $0.contentMode = .scaleAspectFit
    $0.tintColor = UIColor(braveSystemName: .iconDefault)
  }

  private let titleLabel = UILabel().then {
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  private let openButton = BraveButton().then {
    $0.imageView?.contentMode = .scaleAspectFit
    $0.hitTestSlop = UIEdgeInsets(equalInset: -25)
  }

  override var isHighlighted: Bool {
    didSet {
      UIView.animate(
        withDuration: 0.25,
        delay: 0,
        options: [.beginFromCurrentState],
        animations: {
          self.contentView.alpha = self.isHighlighted ? 0.5 : 1.0
        }
      )
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    setTheme()
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .clear

    setTheme()

    contentView.addSubview(stackView)
    [searchImageView, titleLabel, openButton].forEach {
      stackView.addArrangedSubview($0)
    }

    stackView.snp.makeConstraints {
      $0.horizontalEdges.equalToSuperview()
      $0.verticalEdges.equalToSuperview().inset(8)
    }

    searchImageView.snp.makeConstraints {
      $0.size.equalTo(20.0)
    }

    openButton.snp.makeConstraints {
      $0.size.equalTo(20.0)
    }

    openButton.addTarget(self, action: #selector(onOpenButtonPressed), for: .touchUpInside)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func setTitle(_ title: String) {
    titleLabel.text = title
  }

  private func setTheme() {
    titleLabel.do {
      $0.textColor = UIColor(braveSystemName: .textPrimary)
      $0.lineBreakMode = .byTruncatingMiddle

      var sizeCategory = UIApplication.shared.preferredContentSizeCategory
      if sizeCategory.isAccessibilityCategory {
        sizeCategory = .medium
      }
      let traitCollection = UITraitCollection(preferredContentSizeCategory: sizeCategory)
      let font = UIFont.preferredFont(
        for: .subheadline,
        weight: .regular,
        traitCollection: traitCollection
      )
      $0.font = font
    }

    openButton.do {
      $0.setImage(
        UIImage(braveSystemNamed: "leo.arrow.diagonal-up-left")?.template,
        for: .normal
      )
      $0.tintColor = UIColor(braveSystemName: .iconInteractive)
    }
  }

  @objc
  private func onOpenButtonPressed() {
    openButtonActionHandler?()
  }
}
