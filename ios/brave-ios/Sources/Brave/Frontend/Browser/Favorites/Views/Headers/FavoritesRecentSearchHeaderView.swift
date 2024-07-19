// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import UIKit

class FavoritesRecentSearchHeaderView: UICollectionReusableView {

  private let titleLabel = UILabel().then {
    $0.text = Strings.recentSearchSectionTitle
    $0.font = .systemFont(ofSize: 15.0, weight: .semibold)
    $0.textColor = .braveLabel
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  let showMoreButton = UIButton()
  let clearButton = UIButton()

  private let vStackView = UIStackView().then {
    $0.axis = .vertical
  }

  private let hStackView = UIStackView().then {
    $0.spacing = 9.0
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

  private func setTheme() {
    showMoreButton.do {
      $0.setTitle(Strings.recentShowMore, for: .normal)
      $0.setTitleColor(.braveBlurpleTint, for: .normal)
      $0.titleLabel?.font = .systemFont(ofSize: 12.0)
      $0.backgroundColor = .clear
    }

    clearButton.do {
      $0.setTitle(Strings.recentSearchClear, for: .normal)
      $0.setTitleColor(.braveBlurpleTint, for: .normal)
      $0.titleLabel?.font = .systemFont(ofSize: 12.0)
      $0.backgroundColor = .clear
    }
  }

  private func doLayout() {
    addSubview(vStackView)
    vStackView.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview().inset(12.0)
      $0.top.bottom.equalToSuperview().priority(.low)
    }

    let spacer = UIView().then {
      $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
      $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
    }

    vStackView.addArrangedSubview(hStackView)

    [titleLabel, spacer, showMoreButton, clearButton].forEach(hStackView.addArrangedSubview(_:))
    hStackView.setCustomSpacing(0, after: spacer)
    hStackView.setCustomSpacing(15, after: showMoreButton)

    showMoreButton.do {
      $0.setContentHuggingPriority(.required, for: .horizontal)
      $0.setContentCompressionResistancePriority(.required, for: .horizontal)
      $0.contentHorizontalAlignment = .leading
    }

    clearButton.do {
      $0.setContentHuggingPriority(.required, for: .horizontal)
      $0.setContentCompressionResistancePriority(.required, for: .horizontal)
      $0.contentHorizontalAlignment = .trailing
    }

    showMoreButton.snp.makeConstraints {
      $0.height.lessThanOrEqualTo(40.0)
    }

    clearButton.snp.makeConstraints {
      $0.height.lessThanOrEqualTo(40.0)
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    setTheme()
  }

  func setButtonVisibility(showMoreButtonVisible: Bool) {
    showMoreButton.alpha = showMoreButtonVisible ? 1.0 : 0.0
  }

  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    let adjustedBounds = clearButton.frame.inset(
      by: UIEdgeInsets(top: -25.0, left: -25.0, bottom: 0.0, right: -25.0)
    )

    if adjustedBounds.contains(point) {
      return clearButton
    }

    return super.hitTest(point, with: event)
  }

}
