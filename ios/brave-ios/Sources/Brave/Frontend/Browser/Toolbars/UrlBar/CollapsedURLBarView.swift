// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import Foundation
import Shared
import SnapKit
import UIKit

/// A view that displays the tab's secure content state and the URL while scrolling into the page
class CollapsedURLBarView: UIView {
  private let stackView = UIStackView().then {
    $0.spacing = 4
    $0.isUserInteractionEnabled = false
    $0.alignment = .firstBaseline
  }

  private let secureContentStateView = UIButton().then {
    $0.tintAdjustmentMode = .normal
  }

  private let separatorLine = UILabel().then {
    $0.isUserInteractionEnabled = false
    $0.isAccessibilityElement = false
    $0.text = "–"  // en dash
  }

  private let urlLabel = UILabel().then {
    $0.font = .preferredFont(forTextStyle: .caption1)
    $0.textColor = .bravePrimary
    $0.lineBreakMode = .byTruncatingHead
    $0.numberOfLines = 1
    $0.textAlignment = .right
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  var browserColors: any BrowserColors = .standard {
    didSet {
      updateForTraitCollectionAndBrowserColors()
    }
  }

  var isUsingBottomBar: Bool = false {
    didSet {
      setNeedsUpdateConstraints()
    }
  }

  private func updateLockImageView() {
    secureContentStateView.isHidden = !secureContentState.shouldDisplayWarning
    separatorLine.isHidden = secureContentStateView.isHidden
    secureContentStateView.configuration = secureContentStateButtonConfiguration
  }

  private var secureContentStateButtonConfiguration: UIButton.Configuration {
    let clampedTraitCollection = traitCollection.clampingSizeCategory(maximum: .accessibilityLarge)
    var configuration = UIButton.Configuration.plain()
    configuration.preferredSymbolConfigurationForImage = .init(
      font: .preferredFont(forTextStyle: .caption1, compatibleWith: clampedTraitCollection),
      scale: .small
    )
    configuration.buttonSize = .small
    configuration.imagePadding = 4
    configuration.contentInsets = .init(top: 0, leading: 0, bottom: 0, trailing: 0)

    var title = AttributedString(Strings.tabToolbarNotSecureTitle)
    title.font = .preferredFont(forTextStyle: .caption1, compatibleWith: clampedTraitCollection)

    let isTitleVisible =
      !traitCollection.preferredContentSizeCategory.isAccessibilityCategory
      && secureContentState != .mixedContent

    switch secureContentState {
    case .localhost, .secure:
      break
    case .invalidCert:
      configuration.baseForegroundColor = UIColor(braveSystemName: .systemfeedbackErrorIcon)
      if isTitleVisible {
        configuration.attributedTitle = title
      }
      configuration.image = UIImage(braveSystemNamed: "leo.warning.triangle-filled")
    case .missingSSL, .mixedContent:
      configuration.baseForegroundColor = UIColor(braveSystemName: .textTertiary)
      if isTitleVisible {
        configuration.attributedTitle = title
      }
      configuration.image = UIImage(braveSystemNamed: "leo.warning.triangle-filled")
    case .unknown:
      configuration.baseForegroundColor = UIColor(braveSystemName: .iconDefault)
      configuration.image = UIImage(braveSystemNamed: "leo.info.filled")
    }
    return configuration
  }

  var secureContentState: TabSecureContentState = .unknown {
    didSet {
      updateLockImageView()
    }
  }

  var currentURL: URL? {
    didSet {
      urlLabel.text = currentURL.map {
        if let internalURL = InternalURL($0), internalURL.isBasicAuthURL {
          Strings.PageSecurityView.signIntoWebsiteURLBarTitle
        } else {
          URLFormatter.formatURLOrigin(
            forDisplayOmitSchemePathAndTrivialSubdomains: $0.absoluteString
          )
        }
      }
    }
  }

  var isKeyboardVisible: Bool = false {
    didSet {
      setNeedsUpdateConstraints()
      updateConstraints()
    }
  }

  private var topConstraint: Constraint?
  private var bottomConstraint: Constraint?

  override init(frame: CGRect) {
    super.init(frame: frame)

    isUserInteractionEnabled = false
    clipsToBounds = false

    addSubview(stackView)
    stackView.addArrangedSubview(secureContentStateView)
    stackView.addArrangedSubview(separatorLine)
    stackView.addArrangedSubview(urlLabel)

    stackView.snp.makeConstraints {
      topConstraint = $0.top.equalToSuperview().constraint
      bottomConstraint = $0.bottom.equalToSuperview().constraint
      $0.leading.greaterThanOrEqualToSuperview().inset(12)
      $0.trailing.lessThanOrEqualToSuperview().inset(12)
      $0.centerX.equalToSuperview()
    }

    secureContentStateView.configurationUpdateHandler = { [unowned self] button in
      button.configuration = secureContentStateButtonConfiguration
    }

    updateForTraitCollectionAndBrowserColors()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateForTraitCollectionAndBrowserColors()
  }

  private func updateForTraitCollectionAndBrowserColors() {
    let clampedTraitCollection = traitCollection.clampingSizeCategory(maximum: .accessibilityLarge)
    urlLabel.font = .preferredFont(forTextStyle: .caption1, compatibleWith: clampedTraitCollection)
    urlLabel.textColor = browserColors.textPrimary
    separatorLine.font = urlLabel.font
    separatorLine.textColor = browserColors.dividerSubtle
  }

  override func didMoveToWindow() {
    super.didMoveToWindow()
    setNeedsUpdateConstraints()
  }

  override func updateConstraints() {
    super.updateConstraints()

    if isKeyboardVisible && isUsingBottomBar {
      bottomConstraint?.update(inset: 0)
      topConstraint?.update(inset: 0)
    } else {
      let safeAreaInset = window.map(\.safeAreaInsets) ?? .zero
      bottomConstraint?.update(inset: safeAreaInset.top > 0 && !isUsingBottomBar ? 4 : 0)
      topConstraint?.update(inset: safeAreaInset.bottom > 0 && isUsingBottomBar ? 4 : 0)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
