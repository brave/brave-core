// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import Shared
import BraveShared

extension BraveTalkOptInSuccessViewController {
  class View: UIView {

    private let image = UIImageView(image: UIImage(named: "optin_check_circle", in: .module, compatibleWith: nil)!).then {
      $0.contentMode = .scaleAspectFit
    }

    private let title = UILabel().then {
      $0.text = Strings.Rewards.braveTalkRewardsOptInSuccessTitle
      $0.font = .preferredFont(forTextStyle: .title3)
      $0.adjustsFontForContentSizeCategory = true
      $0.textColor = .bravePrimary
      $0.numberOfLines = 0
      $0.textAlignment = .center
    }

    private let body = UILabel().then {
      $0.text = Strings.Rewards.braveTalkRewardsOptInSuccessBody
      $0.font = .preferredFont(forTextStyle: .body)
      $0.adjustsFontForContentSizeCategory = true
      $0.textColor = .braveLabel
      $0.numberOfLines = 0
      $0.textAlignment = .center
    }

    private let optinBackground = UIImageView(image: UIImage(named: "optin_bg", in: .module, compatibleWith: nil)!).then {
      $0.contentMode = .scaleAspectFit
    }

    override init(frame: CGRect) {
      super.init(frame: frame)

      backgroundColor = .braveBackground

      let scrollView = UIScrollView()
      addSubview(scrollView)
      scrollView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }

      let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.spacing = 12
        $0.addStackViewItems(
          .view(image),
          .view(title),
          .view(body))

        $0.layoutMargins = .init(top: 44, left: 32, bottom: 24, right: 32)
        $0.isLayoutMarginsRelativeArrangement = true
      }

      scrollView.addSubview(stackView)

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
  }
}
