// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

class SearchSuggestionPromptCell: UITableViewCell {

  static let identifier = "SearchSuggestionPromptCell"

  struct DesignUX {
    static let paddingX: CGFloat = 15.0
    static let paddingY: CGFloat = 10.0
    static let layoutInsetX: CGFloat = 12.0
    static let layoutInsetY: CGFloat = 15.0
    static let buttonHeight: CGFloat = 40.0
  }

  private let titleLabel = UILabel().then {
    $0.text = Strings.recentSearchSuggestionsTitle
    $0.font = .systemFont(ofSize: 17.0, weight: .semibold)
  }

  private let vStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 11.0
  }

  private let hStackView = UIStackView().then {
    $0.spacing = 9.0
  }

  private let enableButton = UIButton().then {
    $0.setTitle(Strings.recentSearchEnableSuggestions, for: .normal)
    $0.setTitleColor(.braveBackground, for: .normal)
    $0.titleLabel?.font = .systemFont(ofSize: 12.0, weight: .semibold)
    $0.layer.cornerCurve = .continuous
    $0.layer.cornerRadius = DesignUX.buttonHeight / 2.0
    $0.titleEdgeInsets = UIEdgeInsets(
      top: -DesignUX.paddingY,
      left: -DesignUX.paddingX,
      bottom: -DesignUX.paddingY,
      right: -DesignUX.paddingX)
    $0.contentEdgeInsets = UIEdgeInsets(
      top: DesignUX.paddingY,
      left: DesignUX.paddingX,
      bottom: DesignUX.paddingY,
      right: DesignUX.paddingX)
    $0.backgroundColor = .braveBlurpleTint
  }

  private lazy var disableButton = UIButton().then {
    $0.setTitle(Strings.recentSearchDisableSuggestions, for: .normal)
    $0.setTitleColor(.primaryButtonTint, for: .normal)
    $0.titleLabel?.font = .systemFont(ofSize: 12.0, weight: .semibold)
    $0.layer.cornerCurve = .continuous
    $0.layer.cornerRadius = DesignUX.buttonHeight / 2.0
    $0.layer.borderColor = UIColor.braveLabel.cgColor
    $0.layer.borderWidth = 1.0
    $0.titleEdgeInsets = UIEdgeInsets(
      top: -DesignUX.paddingY,
      left: -DesignUX.paddingX,
      bottom: -DesignUX.paddingY,
      right: -DesignUX.paddingX)
    $0.contentEdgeInsets = UIEdgeInsets(
      top: DesignUX.paddingY,
      left: DesignUX.paddingX,
      bottom: DesignUX.paddingY,
      right: DesignUX.paddingX)
    $0.backgroundColor = .clear
  }

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    backgroundColor = .clear

    contentView.addSubview(vStackView)
    vStackView.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview().inset(DesignUX.layoutInsetX)
      $0.top.bottom.equalToSuperview().inset(DesignUX.layoutInsetY)
    }

    let spacer = UIView().then {
      $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
      $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    }

    [titleLabel, hStackView].forEach({
      self.vStackView.addArrangedSubview($0)
    })

    [enableButton, disableButton, spacer].forEach({
      self.hStackView.addArrangedSubview($0)
    })

    enableButton.snp.makeConstraints {
      $0.width.equalTo(disableButton)
      $0.height.equalTo(DesignUX.buttonHeight)
    }

    disableButton.snp.makeConstraints {
      $0.width.equalTo(enableButton)
      $0.height.equalTo(DesignUX.buttonHeight)
    }

    enableButton.addTarget(self, action: #selector(didClickOptInSuggestionsYes), for: .touchUpInside)
    disableButton.addTarget(self, action: #selector(didClickOptInSuggestionsNo), for: .touchUpInside)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  // MARK: -

  var onOptionSelected: ((Bool) -> Void)?

  @objc private func didClickOptInSuggestionsYes() {
    onOptionSelected?(true)
  }

  @objc private func didClickOptInSuggestionsNo() {
    onOptionSelected?(false)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    traitCollection.performAsCurrent {
      self.disableButton.layer.borderColor = UIColor.braveLabel.cgColor
    }
  }
}
