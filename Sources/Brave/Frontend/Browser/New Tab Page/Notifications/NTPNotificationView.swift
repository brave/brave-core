// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

struct NTPNotificationViewConfig {
  var headerText: String?
  var bodyText: (text: String, urlInfo: [String: String], action: ((URL) -> Void)?)?
  var primaryButtonConfig: (text: String, showCoinIcon: Bool, action: (() -> Void))?
  var secondaryButtonConfig: (text: String, action: (() -> Void))?
  let textColor: UIColor

  init(textColor: UIColor) {
    self.textColor = textColor
  }
}

class NTPNotificationView: UIStackView {

  private lazy var titleStackView = UIStackView().then {
    $0.spacing = 10

    let imageView = UIImageView(image: UIImage(named: "brave_rewards_button_enabled", in: .module, compatibleWith: nil)!).then { image in
      image.snp.makeConstraints { make in
        make.size.equalTo(24)
      }
    }

    let title = UILabel().then {
      $0.text = Strings.braveRewardsTitle
      $0.textColor = config.textColor
      $0.font = UIFont.systemFont(ofSize: 16, weight: .semibold)
    }

    [imageView, title].forEach($0.addArrangedSubview(_:))
  }

  lazy var header = UILabel().then {
    $0.text = config.headerText
    $0.textColor = config.textColor

    $0.font = UIFont.systemFont(ofSize: 14, weight: .medium)

    $0.numberOfLines = 0
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.lineBreakMode = .byWordWrapping
  }

  lazy var body = LinkLabel().then {
    $0.font = .systemFont(ofSize: 12.0)
    $0.textColor = config.textColor
    $0.linkColor = UIColor.braveBlurpleTint
    $0.text = config.bodyText?.text

    $0.setContentCompressionResistancePriority(UILayoutPriority(rawValue: 313), for: .vertical)
    $0.textContainerInset = UIEdgeInsets.zero
    $0.textContainer.lineFragmentPadding = 0
  }

  lazy var primaryButton = RoundInterfaceButton(type: .system).then {
    $0.setTitle(config.primaryButtonConfig?.text, for: .normal)
    $0.backgroundColor = .braveBlurpleTint
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
    $0.contentEdgeInsets = UIEdgeInsets(top: 12, left: 25, bottom: 12, right: 25)
    $0.setTitleColor(.white, for: .normal)
    if config.primaryButtonConfig?.showCoinIcon == true {
      $0.setImage(UIImage(named: "turn_rewards_on_money_icon", in: .module, compatibleWith: nil)!, for: .normal)
      $0.imageEdgeInsets = UIEdgeInsets(top: 0, left: -10, bottom: 0, right: 0)
    }
  }

  lazy var secondaryButton = RoundInterfaceButton(type: .system).then {
    $0.setTitle(config.secondaryButtonConfig?.text, for: .normal)
    $0.setTitleColor(config.textColor, for: .normal)
    $0.backgroundColor = .clear
    $0.tintColor = .white
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
  }

  private let config: NTPNotificationViewConfig

  init(config: NTPNotificationViewConfig) {
    self.config = config
    super.init(frame: .zero)

    var views: [UIView] = [titleStackView]

    axis = .vertical
    translatesAutoresizingMaskIntoConstraints = false
    spacing = 16

    if config.headerText != nil {
      views.append(header)
    }

    if config.bodyText != nil {
      views.append(body)

      body.setURLInfo(config.bodyText?.urlInfo ?? [:])
      body.onLinkedTapped = config.bodyText?.action
    }

    if config.primaryButtonConfig != nil {
      let stackView = UIStackView(arrangedSubviews: [
        UIView.spacer(.horizontal, amount: 0),
        primaryButton,
        UIView.spacer(.horizontal, amount: 0),
      ]).then {
        $0.distribution = .equalSpacing
      }

      stackView.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
      views.append(stackView)

      primaryButton.addTarget(self, action: #selector(primaryButtonAction), for: .touchUpInside)
    }

    if config.secondaryButtonConfig != nil {
      let stackView = UIStackView(arrangedSubviews: [
        UIView.spacer(.horizontal, amount: 0),
        secondaryButton,
        UIView.spacer(.horizontal, amount: 0),
      ]).then {
        $0.distribution = .equalSpacing
      }

      stackView.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
      views.append(stackView)

      secondaryButton.addTarget(self, action: #selector(secondaryButtonAction), for: .touchUpInside)
    }

    views.forEach(addArrangedSubview(_:))
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  @objc func primaryButtonAction() {
    config.primaryButtonConfig?.action()
  }

  @objc func secondaryButtonAction() {
    config.secondaryButtonConfig?.action()
  }
}
