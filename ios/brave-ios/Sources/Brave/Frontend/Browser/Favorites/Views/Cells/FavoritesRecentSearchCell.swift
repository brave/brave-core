// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import SnapKit
import UIKit

class FavoritesRecentSearchCell: UICollectionViewCell, CollectionViewReusable {
  static let identifier = "RecentSearchCell"
  var openButtonAction: (() -> Void)?

  private let hStackView = UIStackView().then {
    $0.axis = .horizontal
    $0.alignment = .center
    $0.spacing = 12.0
  }

  private let titleLabel = UILabel().then {
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
  }

  private let openButton = BraveButton(type: .system).then {
    $0.imageView?.contentMode = .scaleAspectFit
    $0.hitTestSlop = UIEdgeInsets(equalInset: -25.0)
  }

  override var isHighlighted: Bool {
    didSet {
      UIView.animate(
        withDuration: 0.25,
        delay: 0,
        options: [.beginFromCurrentState],
        animations: {
          self.alpha = self.isHighlighted ? 0.5 : 1.0
        }
      )
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    setTheme()

    contentView.addSubview(hStackView)
    hStackView.snp.makeConstraints {
      $0.horizontalEdges.equalToSuperview().inset(20)
      $0.verticalEdges.equalToSuperview().inset(10)
    }
    [titleLabel, openButton].forEach(hStackView.addArrangedSubview(_:))

    openButton.snp.makeConstraints {
      $0.size.equalTo(20.0)
    }

    openButton.addTarget(self, action: #selector(onOpenButtonPressed(_:)), for: .touchUpInside)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    setTheme()
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
