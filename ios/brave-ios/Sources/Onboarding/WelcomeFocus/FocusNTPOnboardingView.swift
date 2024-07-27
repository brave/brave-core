// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import SnapKit
import UIKit

public class FocusNTPOnboardingViewController: UIViewController, PopoverContentComponent {

  private let contentStackView = UIStackView().then {
    $0.spacing = 10.0
    $0.axis = .horizontal
    $0.alignment = .leading
    $0.layoutMargins = UIEdgeInsets(equalInset: 16.0)
    $0.isLayoutMarginsRelativeArrangement = true
  }

  private let textStackView = UIStackView().then {
    $0.spacing = 4.0
    $0.axis = .vertical
    $0.alignment = .top
    $0.isLayoutMarginsRelativeArrangement = true
  }

  private let iconImageView: UIImageView = {
    let result = UIImageView(
      image: UIImage(named: "focus-icon-brave", in: .module, compatibleWith: nil)!
    )
    result.tintColor = .white
    result.contentMode = .scaleAspectFit
    result.setContentHuggingPriority(.required, for: .horizontal)
    result.setContentCompressionResistancePriority(.required, for: .horizontal)
    result.setContentCompressionResistancePriority(.required, for: .vertical)
    return result
  }()

  private let titleLabel = UILabel().then {
    $0.textColor = UIColor(braveSystemName: .textInteractive)
    $0.numberOfLines = 0
    $0.font = .preferredFont(for: .callout, weight: .semibold)
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultLow, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let textLabel = UILabel().then {
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.numberOfLines = 0
    $0.font = .preferredFont(forTextStyle: .footnote)
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultLow, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .urlBarIndicatorBackground
    view.addSubview(contentStackView)
    view.addSubview(textStackView)

    contentStackView.addArrangedSubview(iconImageView)
    contentStackView.addArrangedSubview(textStackView)

    textStackView.addArrangedSubview(titleLabel)
    textStackView.addArrangedSubview(textLabel)

    iconImageView.snp.makeConstraints {
      $0.width.height.equalTo(48)
    }

    contentStackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  public func setText(title: String, details: String) {
    titleLabel.text = title
    textLabel.text = details
  }
}

extension UIColor {
  fileprivate static let urlBarIndicatorBackground = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return .white
    } else {
      return UIColor(braveSystemName: .primitiveNeutral5)
    }
  })
}
