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
    static let paddingX = 15.0
    static let paddingY = 10.0
  }

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
  let hideButton = UIButton()

  private let vStackView = UIStackView().then {
    $0.axis = .vertical
  }

  private let hStackView = UIStackView().then {
    $0.spacing = 9.0
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(vStackView)

    vStackView.snp.makeConstraints {
      $0.leading.equalToSuperview().inset(12.0)
      $0.trailing.equalToSuperview().inset(12.0)
      $0.top.bottom.equalToSuperview().priority(.low)
    }

    setTheme()
    doLayout()
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    setTheme()
  }

  private func doLayout() {
    vStackView.arrangedSubviews.forEach({ $0.removeFromSuperview() })
    hStackView.arrangedSubviews.forEach({ $0.removeFromSuperview() })

    let spacer = UIView().then {
      $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
      $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
    }

    [titleLabel, subtitleLabel, hStackView].forEach({
      self.vStackView.addArrangedSubview($0)
    })

    vStackView.setCustomSpacing(12.0, after: titleLabel)
    vStackView.setCustomSpacing(22.0, after: subtitleLabel)

    [showButton, hideButton, spacer].forEach({
      self.hStackView.addArrangedSubview($0)
    })

    showButton.snp.makeConstraints {
      $0.width.equalTo(hideButton)
      $0.height.equalTo(DesignUX.buttonHeight)
    }

    hideButton.snp.makeConstraints {
      $0.width.equalTo(showButton)
      $0.height.equalTo(DesignUX.buttonHeight)
    }
  }

  private func setTheme() {
    let titleEdgeInsets = UIEdgeInsets(
      top: -DesignUX.paddingY,
      left: -DesignUX.paddingX,
      bottom: -DesignUX.paddingY,
      right: -DesignUX.paddingX
    )

    let contentEdgeInsets = UIEdgeInsets(
      top: DesignUX.paddingY,
      left: DesignUX.paddingX,
      bottom: DesignUX.paddingY,
      right: DesignUX.paddingX
    )

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

    hideButton.do {
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
