// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit

class EmptyStateOverlayView: UIView {

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
  
  private let containerView = UIView().then {
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
  }

  required init(title: String? = nil, description: String? = nil, icon: UIImage? = nil) {
    super.init(frame: .zero)

    backgroundColor = .secondaryBraveBackground
    
    addSubview(containerView)
    containerView.snp.makeConstraints {
      $0.centerX.equalToSuperview()
      $0.centerY.equalToSuperview().offset(-50)
      $0.width.equalToSuperview().multipliedBy(0.75)
      $0.size.lessThanOrEqualToSuperview()
    }
    
    containerView.addSubview(iconImageView)
    containerView.addSubview(informationLabel)
    containerView.addSubview(descriptionLabel)
    
    updateFont()
    updateLayoutConstraints()

    if let icon = icon {
      iconImageView.image = icon.template
    }

    if let title = title {
      informationLabel.text = title
    }
    
    if let description = description {
      descriptionLabel.text = description
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    
    updateFont()
    updateLayoutConstraints()
  }
  
  private func updateFont() {
    let clampedTraitCollection = self.traitCollection.clampingSizeCategory(maximum: .accessibilityExtraLarge)
    
    let informationFont = UIFont.preferredFont(forTextStyle: .title3, compatibleWith: clampedTraitCollection)
    let descriptionFont = UIFont.preferredFont(forTextStyle: .subheadline, compatibleWith: clampedTraitCollection)

    informationLabel.font = .systemFont(ofSize: informationFont.pointSize, weight: .medium)
    descriptionLabel.font = descriptionFont
  }
    
  private func updateLayoutConstraints() {
    iconImageView.snp.makeConstraints {
      $0.centerX.equalToSuperview()
      $0.size.equalTo(60)
      $0.top.equalToSuperview().offset(5)
    }

    informationLabel.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview()
      $0.top.equalTo(iconImageView.snp.bottom).offset(15)
    }
        
    descriptionLabel.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview()
      $0.top.equalTo(informationLabel.snp.bottom).offset(15)
      $0.bottom.equalToSuperview().offset(-5)
    }
   }

  func updateInfoLabel(with text: String) {
    informationLabel.text = text
  }
  
  func updateDescriptionLabel(with text: String) {
    descriptionLabel.text = text
  }
}
