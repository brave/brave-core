// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

class SearchFindInPageCell: UICollectionViewCell, CollectionViewReusable {

  private var title: String = ""

  private let stackView = UIStackView().then {
    $0.spacing = 16.0
    $0.alignment = .center
  }

  private let searchImageView = UIImageView().then {
    $0.image = UIImage(braveSystemNamed: "leo.window.search")!.template
    $0.contentMode = .scaleAspectFit
  }

  private let titleLabel = UILabel().then {
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
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

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .clear

    setTheme()

    contentView.addSubview(stackView)
    stackView.addArrangedSubview(searchImageView)
    stackView.addArrangedSubview(titleLabel)

    stackView.snp.makeConstraints {
      $0.horizontalEdges.equalToSuperview().inset(20)
      $0.verticalEdges.equalToSuperview().inset(4)
    }

    searchImageView.snp.makeConstraints {
      $0.size.equalTo(20)
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    setTheme()
  }

  private func setTheme() {
    updateTitleTheme()

    searchImageView.do {
      $0.tintColor = UIColor(braveSystemName: .iconDefault)
    }
  }

  private func updateTitleTheme() {
    titleLabel.textColor = UIColor(braveSystemName: .textPrimary)
    titleLabel.lineBreakMode = .byTruncatingTail

    var sizeCategory = UIApplication.shared.preferredContentSizeCategory
    if sizeCategory.isAccessibilityCategory {
      sizeCategory = .medium
    }
    let traitCollection = UITraitCollection(preferredContentSizeCategory: sizeCategory)
    let boldFont = UIFont.preferredFont(
      for: .subheadline,
      weight: .semibold,
      traitCollection: traitCollection
    )
    let regularFont = UIFont.preferredFont(
      for: .subheadline,
      weight: .regular,
      traitCollection: traitCollection
    )

    let attString = NSMutableAttributedString(
      string: Strings.findInPageFormat,
      attributes: [
        .font: boldFont,
        .foregroundColor: UIColor(braveSystemName: .textTertiary),
      ]
    )
    attString.append(
      NSAttributedString(
        string: "\"\(title)\"",
        attributes: [
          .font: regularFont,
          .foregroundColor: UIColor(braveSystemName: .textPrimary),
        ]
      )
    )
    titleLabel.attributedText = attString
  }

  func setCellTitle(_ title: String) {
    self.title = title
    updateTitleTheme()
  }
}
