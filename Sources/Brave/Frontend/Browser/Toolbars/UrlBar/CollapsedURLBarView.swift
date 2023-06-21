// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveCore
import BraveStrings
import SnapKit

/// A view that displays the tab's secure content state and the URL while scrolling into the page
class CollapsedURLBarView: UIView {
  private let stackView = UIStackView().then {
    $0.spacing = 4
    $0.isUserInteractionEnabled = false
  }
  
  private let lockImageView = ToolbarButton().then {
    $0.setImage(UIImage(braveSystemNamed: "brave.lock.alt", compatibleWith: nil), for: .normal)
    $0.isHidden = true
    $0.tintColor = .bravePrimary
    $0.isAccessibilityElement = true
    $0.imageView?.contentMode = .center
    $0.contentHorizontalAlignment = .center
    $0.accessibilityLabel = Strings.tabToolbarLockImageAccessibilityLabel
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
  }
  
  private let urlLabel = UILabel().then {
    $0.font = .preferredFont(forTextStyle: .caption1)
    $0.textColor = .bravePrimary
    $0.lineBreakMode = .byTruncatingHead
    $0.numberOfLines = 1
    $0.textAlignment = .right
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }
  
  var isUsingBottomBar: Bool = false {
    didSet {
      setNeedsUpdateConstraints()
    }
  }
  
  private func updateLockImageView() {
    lockImageView.isHidden = false
    
    switch secureContentState {
    case .localHost:
      lockImageView.isHidden = true
    case .insecure:
      lockImageView.setImage(UIImage(braveSystemNamed: "leo.info.filled")?
        .withRenderingMode(.alwaysOriginal)
        .withTintColor(.braveErrorLabel), for: .normal)
      lockImageView.accessibilityLabel = Strings.tabToolbarWarningImageAccessibilityLabel
    case .secure, .unknown:
      lockImageView.setImage(UIImage(braveSystemNamed: "brave.lock.alt", compatibleWith: nil), for: .normal)
      lockImageView.accessibilityLabel = Strings.tabToolbarLockImageAccessibilityLabel
    }
  }
  
  var secureContentState: TabSecureContentState = .unknown {
    didSet {
      updateLockImageView()
    }
  }
  
  var currentURL: URL? {
    didSet {
      urlLabel.text = currentURL.map {
        URLFormatter.formatURLOrigin(forDisplayOmitSchemePathAndTrivialSubdomains: $0.absoluteString)
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
    stackView.addArrangedSubview(lockImageView)
    stackView.addArrangedSubview(urlLabel)
    
    stackView.snp.makeConstraints {
      topConstraint = $0.top.equalToSuperview().constraint
      bottomConstraint = $0.bottom.equalToSuperview().constraint
      $0.leading.greaterThanOrEqualToSuperview().inset(12)
      $0.trailing.lessThanOrEqualToSuperview().inset(12)
      $0.centerX.equalToSuperview()
    }
    
    updateForTraitCollection()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateForTraitCollection()
  }
  
  private func updateForTraitCollection() {
    let clampedTraitCollection = traitCollection.clampingSizeCategory(maximum: .accessibilityLarge)
    lockImageView.setPreferredSymbolConfiguration(
      .init(
        pointSize: UIFont.preferredFont(
          forTextStyle: .footnote,
          compatibleWith: clampedTraitCollection
        ).pointSize,
        weight: .semibold
      ),
      forImageIn: .normal
    )
    urlLabel.font = .preferredFont(forTextStyle: .caption1, compatibleWith: clampedTraitCollection)
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
