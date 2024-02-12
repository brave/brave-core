// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import Shared
import BraveShared

extension BraveTalkRewardsOptInViewController {
  class View: UIView {

    let enableRewardsButton = UIButton(type: .system).then {
      $0.titleLabel?.font = .preferredFont(forTextStyle: .headline)
      $0.titleLabel?.textAlignment = .center
      $0.titleLabel?.lineBreakMode = .byClipping
      $0.titleLabel?.numberOfLines = 0

      $0.layer.cornerCurve = .continuous
      $0.contentEdgeInsets = UIEdgeInsets(top: 10, left: 20, bottom: 10, right: 20)

      $0.backgroundColor = .braveLighterBlurple
      $0.setTitle(Strings.Rewards.braveTalkRewardsOptInButtonTitle, for: .normal)
      $0.setTitleColor(.white, for: .normal)
      $0.titleLabel?.adjustsFontForContentSizeCategory = true
    }

    private let image = UIImageView(image: UIImage(named: "rewards_onboarding_cashback", in: .module, compatibleWith: nil)!).then {
      $0.contentMode = .scaleAspectFit

      $0.layer.shadowColor = UIColor.black.cgColor
      $0.layer.shadowOpacity = 0.25
      $0.layer.shadowOffset = CGSize(width: 0, height: 1)
      $0.layer.shadowRadius = 4
    }

    private let title = UILabel().then {
      $0.text = Strings.Rewards.braveTalkRewardsOptInTitle
      $0.font = .preferredFont(forTextStyle: .title3)
      $0.adjustsFontForContentSizeCategory = true
      $0.textColor = .bravePrimary
      $0.numberOfLines = 0
      $0.textAlignment = .center
    }

    private let body = UILabel().then {
      $0.text = Strings.Rewards.braveTalkRewardsOptInBody
      $0.font = .preferredFont(forTextStyle: .body)
      $0.adjustsFontForContentSizeCategory = true
      $0.textColor = .braveLabel
      $0.numberOfLines = 0
      $0.textAlignment = .center
    }

    let disclaimer = LinkLabel().then {
      $0.text = String(
        format: Strings.Rewards.braveTalkRewardsOptInDisclaimer,
        Strings.OBRewardsAgreementDetailLink,
        Strings.privacyPolicy)
      $0.font = .preferredFont(forTextStyle: .caption1)
      $0.adjustsFontForContentSizeCategory = true
      $0.textColor = .braveLabel
      $0.textAlignment = .center
      $0.setURLInfo([
        Strings.OBRewardsAgreementDetailLink: "tos",
        Strings.privacyPolicy: "privacy-policy",
      ])
    }

    private let optinBackground = UIImageView(image: UIImage(named: "optin_bg", in: .module, compatibleWith: nil)!).then {
      $0.contentMode = .scaleAspectFit
    }

    override init(frame: CGRect) {
      super.init(frame: frame)

      backgroundColor = .braveBackground

      let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.spacing = 12
        $0.addStackViewItems(
          .view(image),
          .view(title),
          .view(body),
          .view(enableRewardsButton),
          .view(disclaimer))

        $0.layoutMargins = .init(top: 44, left: 32, bottom: 24, right: 32)
        $0.isLayoutMarginsRelativeArrangement = true
      }

      let scrollView = UIScrollView()
      addSubview(scrollView)

      scrollView.snp.makeConstraints {
        $0.leading.trailing.equalToSuperview()
        $0.top.bottom.equalToSuperview()
      }

      scrollView.addSubview(stackView)

      stackView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }

      scrollView.contentLayoutGuide.snp.makeConstraints {
        $0.width.equalToSuperview()
        $0.top.bottom.equalTo(stackView)
      }

      scrollView.insertSubview(optinBackground, belowSubview: stackView)
      optinBackground.snp.makeConstraints {
        $0.left.top.equalToSuperview().inset(10)
      }
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }

    private func setupButtonConstaints() {
      // This is required in order to support dynamic types.
      // Otherwise the titleLabel do not fit the button's frame.
      enableRewardsButton.titleLabel?.snp.remakeConstraints {
        $0.edges.equalToSuperview()
      }

      let isBigContentSize =
        enableRewardsButton.traitCollection.preferredContentSizeCategory > .extraExtraLarge

      let cappedRadius: CGFloat = 24
      let standardRadius = enableRewardsButton.bounds.height / 2.0

      enableRewardsButton.layer.cornerRadius = isBigContentSize ? cappedRadius : standardRadius
    }

    override func layoutSubviews() {
      super.layoutSubviews()

      setupButtonConstaints()
    }

    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
      super.traitCollectionDidChange(previousTraitCollection)

      if previousTraitCollection?.preferredContentSizeCategory
        != traitCollection.preferredContentSizeCategory {

        DispatchQueue.main.async {
          self.setupButtonConstaints()
        }
      }
    }
  }
}
