// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import UIKit

class FavoritesRecentSearchOptInHeaderView: UICollectionReusableView {
  private struct DesignUX {
    static let buttonHeight = 40.0
    static let buttonCornerRadius = 12.0
    static let paddingX = 15.0
    static let paddingY = 10.0
    static let searchImageSize = CGSize(width: 40.0, height: 40.0)
  }

  let headerLabel = UILabel().then {
    $0.text = Strings.recentSearchSectionHeaderTitle
    $0.font = .systemFont(ofSize: 18, weight: .semibold)
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.setContentHuggingPriority(.required, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let searchImageView = UIImageView().then {
    $0.image = UIImage(braveSystemNamed: "leo.search.history")
    $0.contentMode = .scaleAspectFit
    $0.tintColor = UIColor(braveSystemName: .iconSecondary)
  }

  private let titleLabel = UILabel().then {
    $0.text = Strings.recentSearchSectionTitle
    $0.font = .systemFont(ofSize: 16.0, weight: .semibold)
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.numberOfLines = 0
    $0.textAlignment = .center
    $0.setContentHuggingPriority(.required, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let subtitleLabel = UILabel().then {
    $0.text = Strings.recentSearchSectionDescription
    $0.font = .systemFont(ofSize: 16.0)
    $0.textColor = UIColor(braveSystemName: .textSecondary)
    $0.numberOfLines = 0
    $0.textAlignment = .center
    $0.setContentHuggingPriority(.required, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  let showButton = UIButton()
  let hideButton = UIButton()

  private let vStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 16
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    setTheme()
    doLayout()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    setTheme()
  }

  private func doLayout() {
    addSubview(headerLabel)

    headerLabel.snp.makeConstraints {
      $0.top.equalToSuperview().inset(16.0)
      $0.leading.equalToSuperview().inset(18.0)
      $0.trailing.lessThanOrEqualToSuperview().inset(18.0)
    }

    addSubview(vStackView)

    vStackView.snp.makeConstraints {
      $0.top.equalTo(headerLabel.snp.bottom).inset(-16.0)
      $0.leading.equalToSuperview().inset(4.0)
      $0.bottom.equalToSuperview().inset(4.0)
      $0.trailing.equalToSuperview().inset(4.0)
    }

    vStackView.arrangedSubviews.forEach({ $0.removeFromSuperview() })

    [searchImageView, titleLabel, subtitleLabel, showButton, hideButton].forEach({
      self.vStackView.addArrangedSubview($0)
    })

    searchImageView.snp.makeConstraints {
      $0.height.equalTo(DesignUX.searchImageSize.height)
      $0.width.equalTo(DesignUX.searchImageSize.width)
    }

    showButton.snp.makeConstraints {
      $0.height.equalTo(DesignUX.buttonHeight)
    }

    hideButton.snp.makeConstraints {
      $0.height.equalTo(DesignUX.buttonHeight)
    }
  }

  private func setTheme() {
    self.do {
      $0.backgroundColor = UIColor(braveSystemName: .containerBackground).withAlphaComponent(0.55)
      $0.layer.cornerRadius = 12.0
    }

    vStackView.do {
      $0.backgroundColor = UIColor(braveSystemName: .containerBackground)
      $0.layer.cornerRadius = 12.0
      $0.layoutMargins = .init(top: 16, left: 16, bottom: 16, right: 16)
      $0.isLayoutMarginsRelativeArrangement = true
    }

    showButton.do {
      $0.setTitle(Strings.recentSearchShow, for: .normal)
      $0.setTitleColor(UIColor(braveSystemName: .schemesOnPrimary), for: .normal)
      $0.titleLabel?.font = .systemFont(ofSize: 14.0, weight: .semibold)
      $0.layer.cornerCurve = .continuous
      $0.layer.cornerRadius = DesignUX.buttonCornerRadius
      $0.layer.borderColor = nil
      $0.layer.borderWidth = 0.0
      $0.backgroundColor = UIColor(braveSystemName: .buttonBackground)
    }

    hideButton.do {
      $0.setTitle(Strings.recentSearchHide, for: .normal)
      $0.setTitleColor(UIColor(braveSystemName: .textInteractive), for: .normal)
      $0.titleLabel?.font = .systemFont(ofSize: 14.0, weight: .semibold)
      $0.backgroundColor = .clear
    }
  }
}
