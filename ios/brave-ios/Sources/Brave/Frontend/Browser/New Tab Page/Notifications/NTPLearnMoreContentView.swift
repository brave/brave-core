// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

extension NTPLearnMoreViewController {
  struct NTPNotificationLearnMoreViewConfig {
    let headerText: String
    let tosText: Bool
    let learnMoreButtonText: String
    var headerBodySpacing: CGFloat?
  }

  class NTPLearnMoreContentView: UIStackView {

    weak var delegate: NTPLearnMoreViewDelegate?

    private let looseSpacing: CGFloat = 16
    private let tightSpacing: CGFloat = 8

    private lazy var titleStackView = UIStackView().then {
      $0.spacing = 10

      let imageView = UIImageView(image: UIImage(named: "brave_rewards_button_enabled", in: .module, compatibleWith: nil)!).then { image in
        image.snp.makeConstraints { make in
          make.size.equalTo(24)
        }
      }

      let title = UILabel().then {
        $0.text = Strings.braveRewardsTitle
        $0.textColor = .black
        $0.font = UIFont.systemFont(ofSize: 16, weight: .semibold)
      }

      [imageView, title].forEach($0.addArrangedSubview(_:))
    }

    override func layoutSubviews() {
      super.layoutSubviews()

      let smallHeight = UIScreen.main.bounds.height <= 375
      updateSpacing(amount: smallHeight ? tightSpacing : looseSpacing)
    }

    private func updateSpacing(amount: CGFloat) {
      spacing = amount
      contentStackView.spacing = amount

      if let customHeaderSpacing = config.headerBodySpacing {
        contentStackView.setCustomSpacing(customHeaderSpacing, after: header)
      }
    }

    let contentStackView = UIStackView().then { stackView in
      stackView.axis = .vertical
    }

    private lazy var header = UILabel().then {
      $0.text = config.headerText
      $0.textColor = .black

      $0.font = UIFont.systemFont(ofSize: 14, weight: .medium)

      $0.numberOfLines = 0
      $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
      $0.lineBreakMode = .byWordWrapping
      $0.adjustsFontSizeToFitWidth = true
    }

    private let tosString = Strings.termsOfService.withNonBreakingSpace

    private lazy var tos = detailLinkLabel(
      with:
        String(format: Strings.NTP.turnRewardsTos, tosString)
    ).then {
      $0.setURLInfo([tosString: "tos"])
    }

    private lazy var buttonsStackView = UIStackView().then { stackView in
      func separator() -> UIView {
        UIView().then {
          $0.backgroundColor = UIColor(white: 0.8, alpha: 1.0)
          $0.snp.makeConstraints { v in
            v.height.equalTo(1)
          }
        }
      }

      stackView.axis = .vertical
      stackView.spacing = 8

      [separator(), learnMoreButton, separator(), hideSponsoredImageButton, separator()]
        .forEach(stackView.addArrangedSubview(_:))
    }

    private lazy var learnMoreButton = secondaryButton(title: nil).then {
      $0.addTarget(self, action: #selector(learnMoreButtonAction), for: .touchUpInside)
    }

    private lazy var hideSponsoredImageButton =
      secondaryButton(title: Strings.NTP.hideSponsoredImages).then {
        $0.addTarget(self, action: #selector(hideSponsoredImagesAction), for: .touchUpInside)
      }

    private let config: NTPNotificationLearnMoreViewConfig

    init(config: NTPNotificationLearnMoreViewConfig) {
      self.config = config
      super.init(frame: .zero)

      var views: [UIView] = [titleStackView]

      axis = .vertical
      translatesAutoresizingMaskIntoConstraints = false

      views.append(header)

      if config.tosText {
        views.append(tos)

        tos.onLinkedTapped = { [weak self] url in
          self?.delegate?.tosTapped()
        }
      }

      let mainContentStackView = UIStackView().then {
        $0.addArrangedSubview(UIView.spacer(.horizontal, amount: 32))
        views.forEach(contentStackView.addArrangedSubview(_:))

        $0.addArrangedSubview(contentStackView)
        $0.addArrangedSubview(UIView.spacer(.horizontal, amount: 32))
      }

      addArrangedSubview(mainContentStackView)
      addArrangedSubview(buttonsStackView)

      learnMoreButton.setTitle(config.learnMoreButtonText, for: .normal)
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }

    // MARK: - Actions

    @objc func learnMoreButtonAction() {
      delegate?.learnMoreTapped()
    }

    @objc func hideSponsoredImagesAction() {
      delegate?.hideSponsoredImagesTapped()
    }
  }
}

// MARK: - View helpers

private func detailLinkLabel(with text: String) -> LinkLabel {
  LinkLabel().then {
    $0.font = .systemFont(ofSize: 12.0)
    $0.textColor = UIColor(white: 0, alpha: 0.7)
    $0.linkColor = UIColor.braveBlurpleTint
    $0.text = text
    $0.textContainerInset = .zero
    $0.textContainer.lineFragmentPadding = 0
  }
}

private func secondaryButton(title: String?) -> UIButton {
  UIButton().then {
    if let title = title {
      $0.setTitle(title, for: .normal)
    }
    $0.setTitleColor(.black, for: .normal)
    $0.contentHorizontalAlignment = .left
    $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 32, bottom: 0, right: 32)
    $0.titleLabel?.font = UIFont.systemFont(ofSize: 17, weight: .regular)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.titleLabel?.adjustsFontSizeToFitWidth = true
  }
}
