// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit

class EmptyStateOverlayView: UIView {
  
  private struct UX {
    static let titleEdgeInsets = UIEdgeInsets(top: -10.0, left: -35.0, bottom: -10.0, right: -35.0)
    static let contentEdgeInsets = UIEdgeInsets(top: 10.0, left: 35.0, bottom: 10.0, right: 35.0)
    static let buttonHeight = 40.0
  }

  private let iconImageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.tintColor = .braveLabel
  }

  private let informationLabel = UILabel().then {
    $0.textAlignment = .center
    $0.textColor = .braveLabel
    $0.numberOfLines = 0
  }
  
  private let descriptionLabel = UILabel().then {
    $0.textAlignment = .center
    $0.textColor = .braveLabel
    $0.numberOfLines = 0
  }
  
  private let actionButton = UIButton().then {
    $0.setTitleColor(.white, for: .normal)
    $0.layer.cornerCurve = .continuous
    $0.layer.cornerRadius = UX.buttonHeight / 2.0
    $0.titleEdgeInsets = UX.titleEdgeInsets
    $0.contentEdgeInsets = UX.contentEdgeInsets
    $0.backgroundColor = .braveBlurple
  }
  
  private let actionDescriptionLabel = UILabel().then {
    $0.textAlignment = .center
    $0.textColor = .braveLabel
    $0.numberOfLines = 0
  }
  
  private let containerView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .center
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
  }
  
  private var actionHandler: (() -> Void)?

  required init(
    title: String? = nil,
    description: String? = nil,
    icon: UIImage? = nil,
    buttonText: String? = nil,
    action: (() -> Void)? = nil,
    actionDescription: String? = nil) {
    super.init(frame: .zero)

    backgroundColor = .secondaryBraveBackground
    
    addSubview(containerView)
    containerView.snp.makeConstraints {
      $0.centerX.equalToSuperview()
      $0.centerY.equalToSuperview().offset(-50)
      $0.width.equalToSuperview().multipliedBy(0.75)
      $0.size.lessThanOrEqualToSuperview()
    }
    
    if let icon = icon {
      iconImageView.image = icon
      containerView.addArrangedSubview(iconImageView)
      
      iconImageView.snp.makeConstraints {
        $0.size.equalTo(60)
       }
      
      containerView.setCustomSpacing(35, after: iconImageView)
    }
    
    if let title = title {
      informationLabel.text = title
      containerView.addArrangedSubview(informationLabel)
      containerView.setCustomSpacing(25, after: informationLabel)
    }
    
    if let description = description {
      descriptionLabel.text = description
      containerView.addArrangedSubview(descriptionLabel)
      containerView.setCustomSpacing(45, after: descriptionLabel)
    }
    
    if let buttonText = buttonText {
      actionButton.setTitle(buttonText, for: .normal)
      actionButton.addTarget(self, action: #selector(tappedActionButton), for: .touchUpInside)
      containerView.addArrangedSubview(actionButton)
      containerView.setCustomSpacing(25, after: actionButton)
    }
    
    if let actionDescription = actionDescription {
      actionDescriptionLabel.text = actionDescription
      containerView.addArrangedSubview(actionDescriptionLabel)
    }
    
    if let action = action {
      actionHandler = action
    }
    
    updateFont()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    
    updateFont()
  }
  
  private func updateFont() {
    let clampedTraitCollection = self.traitCollection.clampingSizeCategory(maximum: .accessibilityExtraLarge)
    
    let informationFont = UIFont.preferredFont(forTextStyle: .title3, compatibleWith: clampedTraitCollection)
    let descriptionFont = UIFont.preferredFont(forTextStyle: .subheadline, compatibleWith: clampedTraitCollection)
    let buttonFont = UIFont.preferredFont(forTextStyle: .headline, compatibleWith: clampedTraitCollection)

    informationLabel.font = .systemFont(ofSize: informationFont.pointSize, weight: .medium)
    descriptionLabel.font = descriptionFont
    actionButton.titleLabel?.font = .systemFont(ofSize: buttonFont.pointSize)
    actionDescriptionLabel.font = descriptionFont
  }

  func updateInfoLabel(with text: String) {
    informationLabel.text = text
  }
  
  func updateDescriptionLabel(with text: String) {
    descriptionLabel.text = text
  }
  
  @objc private func tappedActionButton() {
    actionHandler?()
  }
}
