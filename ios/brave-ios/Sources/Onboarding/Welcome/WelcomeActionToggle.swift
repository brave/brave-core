// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import SnapKit
import UIKit

class WelcomeShareActionToggle: BlockedAdsStackView {

  struct DesignDetails {
    let defaultSpacing: CGFloat = 10
    let defaultEdgeInset = UIEdgeInsets(equalInset: 15)
    let defaultCorderRadius: CGFloat = 10
    let defaultFont: UIFont = .preferredFont(for: .body, weight: .regular)
  }

  var text: String? = "" {
    didSet {
      descriptionLabel.text = text
    }
  }

  var font: UIFont {
    didSet {
      descriptionLabel.font = font
    }
  }

  var isOn: Bool = true {
    didSet {
      shareToggle.isOn = isOn
    }
  }

  var onToggleChanged: ((Bool) -> Void)?

  private let descriptionLabel = UILabel().then {
    $0.textColor = .bravePrimary
    $0.textAlignment = .left
    $0.numberOfLines = 0
    $0.minimumScaleFactor = 0.5
    $0.adjustsFontSizeToFitWidth = true
    $0.setContentHuggingPriority(.defaultLow, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private(set) lazy var shareToggle = UISwitch().then {
    $0.addTarget(self, action: #selector(didToggleShare), for: .valueChanged)
    $0.onTintColor = .braveBlurpleTint
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  init(design: DesignDetails = DesignDetails()) {
    self.font = design.defaultFont
    super.init(edgeInsets: design.defaultEdgeInset, spacing: design.defaultSpacing)

    addBackground(color: .black.withAlphaComponent(0.025), cornerRadius: design.defaultCorderRadius)

    addStackViewItems(
      .view(descriptionLabel),
      .view(shareToggle)
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  @objc func didToggleShare(_ toggle: UISwitch) {
    onToggleChanged?(toggle.isOn)
  }
}
