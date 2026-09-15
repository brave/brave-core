// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import UIKit

class SearchActionsCell: UICollectionViewCell, CollectionViewReusable {
  private struct DesignUX {
    static let buttonCornerRadius = 12.0
    static let searchImageSize = CGSize(width: 64.0, height: 64.0)
  }

  let imageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.image = UIImage(named: "recent-search-opt-in", in: .module, with: nil)
  }

  let titleLabel = UILabel()

  let subtitleLabel = UILabel()

  let primaryButton = BraveButton(type: .system)
  let secondaryButton = BraveButton(type: .system)

  private let topHStackView = UIStackView().then {
    $0.alignment = .top
  }

  private let titleVStackView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .leading
    $0.spacing = 8
  }

  private let buttonHStackView = UIStackView().then {
    $0.spacing = 16
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    setTheme()
    doLayout()

    registerForTraitChanges([UITraitPreferredContentSizeCategory.self]) { (self: Self, _) in
      self.setTheme()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    backgroundColor = .clear
  }

  private func doLayout() {
    [topHStackView, buttonHStackView].forEach {
      contentView.addSubview($0)
    }
    topHStackView.snp.makeConstraints {
      $0.top.leading.trailing.equalToSuperview().inset(16)
    }
    buttonHStackView.snp.makeConstraints {
      $0.top.equalTo(topHStackView.snp.bottom).offset(16)
      $0.leading.bottom.trailing.equalToSuperview().inset(16)
    }

    [titleLabel, subtitleLabel].forEach {
      titleVStackView.addArrangedSubview($0)
    }

    [titleVStackView, imageView].forEach {
      topHStackView.addArrangedSubview($0)
    }

    [primaryButton, secondaryButton].forEach {
      buttonHStackView.addArrangedSubview($0)
    }

    imageView.snp.makeConstraints {
      $0.height.equalTo(DesignUX.searchImageSize.height)
      $0.width.equalTo(DesignUX.searchImageSize.width)
    }

    primaryButton.snp.makeConstraints {
      $0.width.equalTo(secondaryButton.snp.width)
    }
  }

  private func setTheme() {
    var sizeCategory = UIApplication.shared.preferredContentSizeCategory
    if sizeCategory.isAccessibilityCategory {
      sizeCategory = .medium
    }
    let traitCollection = UITraitCollection(preferredContentSizeCategory: sizeCategory)

    titleLabel.do {
      $0.textColor = UIColor(braveSystemName: .textPrimary)
      $0.numberOfLines = 0

      let font = UIFont.preferredFont(
        for: .body,
        weight: .semibold,
        traitCollection: traitCollection
      )
      $0.font = font
    }

    subtitleLabel.do {
      $0.textColor = UIColor(braveSystemName: .textSecondary)
      $0.numberOfLines = 0

      let font = UIFont.preferredFont(
        for: .body,
        weight: .regular,
        traitCollection: traitCollection
      )
      $0.font = font
    }

    let buttonTitleFont = UIFont.preferredFont(
      for: .subheadline,
      weight: .semibold,
      traitCollection: traitCollection
    )
    var primaryConfiguration = UIButton.Configuration.plain()
    primaryConfiguration.baseBackgroundColor = UIColor(braveSystemName: .buttonBackground)
    primaryConfiguration.baseForegroundColor = UIColor(braveSystemName: .schemesOnPrimary)
    primaryConfiguration.cornerStyle = .capsule
    primaryConfiguration.background.backgroundColor = UIColor(braveSystemName: .buttonBackground)
    primaryConfiguration.contentInsets = NSDirectionalEdgeInsets(
      top: 12,
      leading: 12,
      bottom: 12,
      trailing: 12
    )
    primaryConfiguration.titleTextAttributesTransformer = UIConfigurationTextAttributesTransformer {
      incoming in
      var outgoing = incoming
      outgoing.font = buttonTitleFont
      return outgoing
    }
    primaryButton.configuration = primaryConfiguration

    var secondaryConfiguration = UIButton.Configuration.plain()
    secondaryConfiguration.baseBackgroundColor = .clear
    secondaryConfiguration.baseForegroundColor = UIColor(braveSystemName: .textInteractive)
    secondaryConfiguration.contentInsets = primaryConfiguration.contentInsets
    secondaryConfiguration.titleTextAttributesTransformer =
      primaryConfiguration.titleTextAttributesTransformer
    secondaryButton.configuration = secondaryConfiguration
  }
}
