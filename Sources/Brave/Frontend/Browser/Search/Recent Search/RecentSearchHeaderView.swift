// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveUI
import Shared

class RecentSearchHeaderView: UICollectionReusableView {
  private struct DesignUX {
    static let buttonHeight = 40.0
    static let paddingX = 15.0
    static let paddingY = 10.0
  }

  private var showRecentSearches = false

  private let titleLabel = UILabel().then {
    $0.text = Strings.recentSearchSectionTitle
    $0.font = .systemFont(ofSize: 15.0, weight: .semibold)
    $0.textColor = .braveLabel
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  private let subtitleLabel = UILabel().then {
    $0.text = Strings.recentSearchSectionDescription
    $0.font = .systemFont(ofSize: 13.0)
    $0.textColor = .braveLabel
    $0.numberOfLines = 0
    $0.lineBreakMode = .byWordWrapping
    $0.setContentHuggingPriority(.required, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  let showButton = UIButton()
  let hideClearButton = UIButton()

  private let vStackView = UIStackView().then {
    $0.axis = .vertical
  }

  private let hStackView = UIStackView().then {
    $0.spacing = 9.0
  }

  private let hButtonStackView = UIStackView().then {
    $0.spacing = 9.0
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    addSubview(vStackView)
    vStackView.snp.makeConstraints {
      $0.leading.equalToSuperview().inset(12.0)
      $0.trailing.equalToSuperview().inset(12.0)
      $0.top.bottom.equalToSuperview().priority(.low)
    }

    resetLayout(showRecentSearches: showRecentSearches)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  func setButtonVisibility(showButtonVisible: Bool, clearButtonVisible: Bool) {
    showButton.alpha = showButtonVisible ? 1.0 : 0.0
    hideClearButton.alpha = clearButtonVisible ? 1.0 : 0.0
  }

  func resetLayout(showRecentSearches: Bool) {
    self.showRecentSearches = showRecentSearches
    themeViews()
    doLayout()
  }

  private func doLayout() {
    vStackView.arrangedSubviews.forEach({ $0.removeFromSuperview() })
    hStackView.arrangedSubviews.forEach({ $0.removeFromSuperview() })

    let spacer = UIView().then {
      $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
      $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
    }

    if showRecentSearches {
      vStackView.addArrangedSubview(hStackView)

      [titleLabel, spacer, showButton, hideClearButton].forEach(hStackView.addArrangedSubview(_:))
      hStackView.setCustomSpacing(0, after: spacer)
      hStackView.setCustomSpacing(15, after: showButton)

      showButton.do {
        $0.setContentHuggingPriority(.required, for: .horizontal)
        $0.setContentCompressionResistancePriority(.required, for: .horizontal)
        $0.contentHorizontalAlignment = .leading
      }

      hideClearButton.do {
        $0.setContentHuggingPriority(.required, for: .horizontal)
        $0.setContentCompressionResistancePriority(.required, for: .horizontal)
        $0.contentHorizontalAlignment = .trailing
      }

      showButton.snp.makeConstraints {
        $0.height.lessThanOrEqualTo(DesignUX.buttonHeight)
      }

      hideClearButton.snp.makeConstraints {
        $0.height.lessThanOrEqualTo(DesignUX.buttonHeight)
      }
    } else {
      [titleLabel, subtitleLabel, hStackView].forEach({
        self.vStackView.addArrangedSubview($0)
      })

      vStackView.setCustomSpacing(12.0, after: titleLabel)
      vStackView.setCustomSpacing(22.0, after: subtitleLabel)

      [showButton, hideClearButton, spacer].forEach({
        self.hStackView.addArrangedSubview($0)
      })

      showButton.snp.makeConstraints {
        $0.width.equalTo(hideClearButton)
        $0.height.equalTo(DesignUX.buttonHeight)
      }

      hideClearButton.snp.makeConstraints {
        $0.width.equalTo(showButton)
        $0.height.equalTo(DesignUX.buttonHeight)
      }
    }
  }

  private func themeViews() {
    let titleEdgeInsets = UIEdgeInsets(
      top: -DesignUX.paddingY,
      left: -DesignUX.paddingX,
      bottom: -DesignUX.paddingY,
      right: -DesignUX.paddingX)

    let contentEdgeInsets = UIEdgeInsets(
      top: DesignUX.paddingY,
      left: DesignUX.paddingX,
      bottom: DesignUX.paddingY,
      right: DesignUX.paddingX)

    if showRecentSearches {
      showButton.do {
        $0.setTitle(Strings.recentShowMore, for: .normal)
        $0.setTitleColor(.braveBlurpleTint, for: .normal)
        $0.titleLabel?.font = .systemFont(ofSize: 12.0)
        $0.layer.cornerRadius = 0.0
        $0.layer.borderColor = nil
        $0.layer.borderWidth = 0.0
        $0.titleEdgeInsets = .zero
        $0.contentEdgeInsets = .zero
        $0.backgroundColor = .clear
      }

      hideClearButton.do {
        $0.setTitle(Strings.recentSearchClear, for: .normal)
        $0.setTitleColor(.braveBlurpleTint, for: .normal)
        $0.titleLabel?.font = .systemFont(ofSize: 12.0)
        $0.layer.cornerRadius = 0.0
        $0.layer.borderColor = nil
        $0.layer.borderWidth = 0.0
        $0.titleEdgeInsets = .zero
        $0.contentEdgeInsets = .zero
        $0.backgroundColor = .clear
      }
    } else {
      showButton.do {
        $0.setTitle(Strings.recentSearchShow, for: .normal)
        $0.setTitleColor(.white, for: .normal)
        $0.titleLabel?.font = .systemFont(ofSize: 12.0, weight: .semibold)
        $0.layer.cornerCurve = .continuous
        $0.layer.cornerRadius = DesignUX.buttonHeight / 2.0
        $0.layer.borderColor = nil
        $0.layer.borderWidth = 0.0
        $0.titleEdgeInsets = titleEdgeInsets
        $0.contentEdgeInsets = contentEdgeInsets
        $0.backgroundColor = .braveBlurpleTint
      }

      hideClearButton.do {
        $0.setTitle(Strings.recentSearchHide, for: .normal)
        $0.setTitleColor(.primaryButtonTint, for: .normal)
        $0.titleLabel?.font = .systemFont(ofSize: 12.0, weight: .semibold)
        $0.layer.cornerCurve = .continuous
        $0.layer.cornerRadius = DesignUX.buttonHeight / 2.0
        $0.layer.borderColor = UIColor.braveLabel.cgColor
        $0.layer.borderWidth = 1.0
        $0.titleEdgeInsets = titleEdgeInsets
        $0.contentEdgeInsets = contentEdgeInsets
        $0.backgroundColor = .clear
      }
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    themeViews()
  }
  
  public override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    let adjustedBounds = hideClearButton.frame.inset(by: UIEdgeInsets(top: -25.0, left: -25.0, bottom: 0.0, right: -25.0))

    if adjustedBounds.contains(point) {
      return hideClearButton
    }

    return super.hitTest(point, with: event)
  }
}
