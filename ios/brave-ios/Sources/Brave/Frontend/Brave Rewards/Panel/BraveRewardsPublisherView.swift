// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared
import UIKit

class BraveRewardsPublisherView: UIStackView {

  struct UX {
    static let hostLabelFontSize: CGFloat = 19.0
  }

  let faviconImageView = UIImageView().then {
    $0.snp.makeConstraints {
      $0.size.equalTo(24)
    }
    $0.layer.cornerRadius = 4
    $0.layer.cornerCurve = .continuous
    $0.clipsToBounds = true
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }

  let hostLabel = UILabel().then {
    $0.font = .systemFont(ofSize: UX.hostLabelFontSize)
    $0.textAlignment = .center
    $0.numberOfLines = 0
    $0.textColor = .braveLabel
    // Stop it from becoming like 10 lines with 1 letter per line
    $0.setContentCompressionResistancePriority(.init(999), for: .horizontal)
  }

  let bodyLabel = UILabel().then {
    $0.text = Strings.Rewards.unverifiedPublisher
    $0.numberOfLines = 0
    $0.textAlignment = .center
    $0.font = .systemFont(ofSize: 16)
    $0.textColor = .braveLabel
  }

  let learnMoreButton = BraveButton(type: .system).then {
    $0.setTitle(Strings.learnMore, for: .normal)
    $0.tintColor = .braveBlurpleTint
    $0.isHidden = true
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    axis = .vertical
    alignment = .center
    spacing = 8
    isLayoutMarginsRelativeArrangement = true
    layoutMargins = UIEdgeInsets(top: 10, left: 0, bottom: 10, right: 0)

    addStackViewItems(
      .view(
        UIStackView(arrangedSubviews: [faviconImageView, hostLabel]).then {
          $0.spacing = 8
          $0.alignment = .center
        }),
      .view(
        UIStackView(arrangedSubviews: [bodyLabel]).then {
          $0.isLayoutMarginsRelativeArrangement = true
          $0.layoutMargins = UIEdgeInsets(top: 0, left: 20, bottom: 0, right: 20)
        }),
      .customSpace(4),
      .view(learnMoreButton)
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
