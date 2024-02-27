// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import SnapKit
import UIKit

public class WelcomeNTPOnboardingController: UIViewController, PopoverContentComponent {
  private let textStackView = UIStackView().then {
    $0.spacing = 8.0
    $0.alignment = .top
  }

  private let contentStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 16.0
    $0.alignment = .fill
    $0.layoutMargins = UIEdgeInsets(equalInset: 20.0)
    $0.isLayoutMarginsRelativeArrangement = true
  }

  private let iconView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.image = UIImage(named: "welcome-view-ntp-logo", in: .module, compatibleWith: nil)!
    $0.snp.makeConstraints {
      $0.size.equalTo(40)
    }
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  private let textLabel = UILabel().then {
    $0.numberOfLines = 0
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultLow, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let button = RoundInterfaceButton(type: .custom).then {
    $0.setTitleColor(.white, for: .normal)
    $0.backgroundColor = .braveBlurpleTint
    $0.titleLabel?.numberOfLines = 0
    $0.titleLabel?.minimumScaleFactor = 0.7
    $0.titleLabel?.adjustsFontSizeToFitWidth = true
    $0.contentEdgeInsets = .init(top: 0, left: 16, bottom: 0, right: 16)
  }

  public var buttonText: String?

  public var buttonTapped: (() -> Void)?

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.addSubview(contentStackView)
    textStackView.addArrangedSubview(iconView)
    textStackView.addArrangedSubview(textLabel)

    contentStackView.addArrangedSubview(textStackView)
    if let buttonText = buttonText {
      button.setTitle(buttonText, for: .normal)
      button.snp.makeConstraints {
        $0.height.equalTo(44.0)
      }
      button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
      contentStackView.addArrangedSubview(button)
    }

    contentStackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  @objc func buttonAction() {
    buttonTapped?()
    dismiss(animated: true)
  }

  public func setText(title: String? = nil, details: String) {
    let attributedString = NSMutableAttributedString()
    if let title = title {
      attributedString.append(
        NSAttributedString(
          string: "\(title)\n",
          attributes: [
            .font: UIFont.preferredFont(forTextStyle: .headline)
          ]
        )
      )
    }

    attributedString.append(
      NSAttributedString(
        string: details,
        attributes: [
          .font: UIFont.preferredFont(forTextStyle: .body)
        ]
      )
    )

    textLabel.attributedText = attributedString
  }

  func maskedPointerView(icon: UIImage, tint: UIColor?) -> UIView {
    let view = UIView().then {
      $0.backgroundColor = .braveBackground
      $0.layer.masksToBounds = true
      $0.layer.cornerCurve = .continuous
    }

    let imageView = UIImageView().then {
      $0.image = icon
      $0.contentMode = .center
      $0.tintColor = tint
    }

    view.addSubview(imageView)
    imageView.snp.makeConstraints {
      $0.center.equalToSuperview()
      $0.width.equalTo(view)
      $0.height.equalTo(view)
    }

    return view
  }
}

public class WelcomeOmniBoxOnboardingController: UIViewController, PopoverContentComponent {

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
