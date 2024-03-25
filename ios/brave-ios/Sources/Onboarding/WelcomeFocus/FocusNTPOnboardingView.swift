// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import SnapKit
import UIKit

import Foundation

public class FocusNTPOnboardingViewController: UIViewController, PopoverContentComponent {

  private let stackView = UIStackView().then {
    $0.spacing = 20.0
    $0.axis = .vertical
    $0.alignment = .top
    $0.layoutMargins = UIEdgeInsets(top: 20, left: 20, bottom: 32, right: 20)
    $0.isLayoutMarginsRelativeArrangement = true
  }

  private let titleLabel = UILabel().then {
    $0.textColor = .bravePrimary.resolvedColor(with: .init(userInterfaceStyle: .dark))
    $0.numberOfLines = 0
    $0.font = UIFont.preferredFont(forTextStyle: .subheadline)
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultLow, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let textLabel = UILabel().then {
    $0.textColor = .bravePrimary.resolvedColor(with: .init(userInterfaceStyle: .dark))
    $0.numberOfLines = 0
    $0.font = UIFont.preferredFont(forTextStyle: .title2)
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultLow, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let gradientView = BraveGradientView.gradient01

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.addSubview(gradientView)

    gradientView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    gradientView.addSubview(stackView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(textLabel)

    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  public func setText(title: String, details: String) {
    titleLabel.text = title
    textLabel.text = details
  }
}
