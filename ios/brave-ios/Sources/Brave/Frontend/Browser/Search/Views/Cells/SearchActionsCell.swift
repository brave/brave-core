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

  let titleLabel = UILabel().then {
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.numberOfLines = 0
    $0.font = .preferredFont(for: .body, weight: .semibold)
  }

  let subtitleLabel = UILabel().then {
    $0.textColor = UIColor(braveSystemName: .textSecondary)
    $0.numberOfLines = 0
    $0.font = .preferredFont(for: .body, weight: .regular)
  }

  let primaryButton = UIButton()
  let secondaryButton = UIButton()

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
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    backgroundColor = .clear
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    setTheme()
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
    primaryButton.do {
      $0.layer.cornerCurve = .continuous
      $0.layer.cornerRadius = DesignUX.buttonCornerRadius
      $0.layer.borderColor = nil
      $0.layer.borderWidth = 0.0
      $0.backgroundColor = UIColor(braveSystemName: .buttonBackground)
      $0.titleLabel?.font = .preferredFont(for: .subheadline, weight: .semibold)
      $0.setTitleColor(
        UIColor(braveSystemName: .schemesOnPrimary),
        for: .normal
      )
      $0.contentEdgeInsets = UIEdgeInsets(top: 12, left: 12, bottom: 12, right: 12)
    }

    secondaryButton.do {
      $0.backgroundColor = .clear
      $0.titleLabel?.font = .preferredFont(for: .subheadline, weight: .semibold)
      $0.setTitleColor(UIColor(braveSystemName: .schemesOnPrimary), for: .normal)
      $0.setTitleColor(UIColor(braveSystemName: .textInteractive), for: .normal)
      $0.contentEdgeInsets = UIEdgeInsets(top: 12, left: 12, bottom: 12, right: 12)
    }
  }
}
