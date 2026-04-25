// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import UIKit

class FavoritesRecentSearchHeaderView: UICollectionReusableView {

  private let label = UILabel()

  let clearButton = UIButton()

  private let hStackView = UIStackView().then {
    $0.spacing = 8.0
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
    var sizeCategory = UIApplication.shared.preferredContentSizeCategory
    if sizeCategory.isAccessibilityCategory {
      sizeCategory = .large
    }
    let traitCollection = UITraitCollection(preferredContentSizeCategory: sizeCategory)

    label.do {
      $0.text = Strings.recentSearchSectionHeaderTitle
      $0.textColor = UIColor(braveSystemName: .textPrimary)
      let font = UIFont.preferredFont(
        for: .body,
        weight: .semibold,
        traitCollection: traitCollection
      )
      $0.font = font
    }

    clearButton.do {
      $0.setTitle(Strings.recentSearchClear, for: .normal)
      $0.setTitleColor(UIColor(braveSystemName: .textInteractive), for: .normal)
      $0.backgroundColor = .clear
      let font = UIFont.preferredFont(
        for: .subheadline,
        weight: .semibold,
        traitCollection: traitCollection
      )
      $0.titleLabel?.font = font
    }
  }

  private func doLayout() {
    addSubview(hStackView)
    hStackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    [label, clearButton].forEach(hStackView.addArrangedSubview(_:))

    clearButton.do {
      $0.setContentHuggingPriority(.required, for: .horizontal)
      $0.setContentCompressionResistancePriority(.required, for: .horizontal)
      $0.contentHorizontalAlignment = .trailing
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    setTheme()
  }

  func setButtonVisibility(showClearButtonVisible: Bool) {
    clearButton.alpha = showClearButtonVisible ? 1.0 : 0.0
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
