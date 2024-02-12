// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import UIKit

extension TabTrayController {
  class TabTrayPrivateModeInfoView: UIView {
    private struct UX {
      static let titleColor = UIColor.braveLabel
      static let titleFont = UIFont.systemFont(ofSize: 20, weight: .semibold)
      static let descriptionColor = UIColor.secondaryBraveLabel
      static let descriptionFont = UIFont.systemFont(ofSize: 14)
      static let learnMoreFont = UIFont.systemFont(ofSize: 17, weight: .medium)
      static let textMargin = 40.0
      static let minBottomMargin = 15.0
      static let stackViewSpacing = 15.0
      static let scrollViewTopSpacing = 44.0
    }

    let scrollView = UIScrollView().then {
      $0.alwaysBounceVertical = true
      $0.indicatorStyle = .white
    }

    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = UX.stackViewSpacing
    }

    let titleLabel = UILabel().then {
      $0.textColor = UX.titleColor
      $0.font = UX.titleFont
      $0.textAlignment = .center
      $0.text = Strings.privateBrowsing
      $0.adjustsFontSizeToFitWidth = true
      $0.minimumScaleFactor = 0.75
    }

    let descriptionLabel = UILabel().then {
      $0.textColor = UX.descriptionColor
      $0.font = UX.descriptionFont
      $0.text = Strings.privateTabBody
      $0.numberOfLines = 0
    }

    let detailsLabel = UILabel().then {
      $0.textColor = UX.descriptionColor
      $0.font = UX.descriptionFont
      $0.text = Strings.privateTabDetails
      $0.isHidden = true
      $0.numberOfLines = 0
    }

    let learnMoreButton = UIButton(type: .system).then {
      $0.setTitle(Strings.privateTabLink, for: [])
      $0.setTitleColor(.braveBlurpleTint, for: [])
      $0.titleLabel?.font = UX.learnMoreFont
      $0.titleLabel?.numberOfLines = 0
    }

    let iconImageView = UIImageView(image: UIImage(named: "private_glasses", in: .module, compatibleWith: nil)!.template).then {
      $0.contentMode = .center
      $0.setContentHuggingPriority(.required, for: .vertical)
      $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
      $0.tintColor = .braveLabel
    }

    override init(frame: CGRect) {
      super.init(frame: frame)

      addSubview(scrollView)
      scrollView.addSubview(stackView)
      stackView.addArrangedSubview(iconImageView)
      stackView.addArrangedSubview(titleLabel)
      stackView.addArrangedSubview(descriptionLabel)
      stackView.addArrangedSubview(detailsLabel)
      stackView.addArrangedSubview(learnMoreButton)

      stackView.setCustomSpacing(UX.stackViewSpacing * 2.0, after: iconImageView)

      scrollView.snp.makeConstraints {
        $0.edges.equalTo(self.snp.edges)
      }
      
      scrollView.contentLayoutGuide.snp.makeConstraints {
        $0.width.equalTo(self)
        $0.top.equalTo(self.stackView).offset(2 * UX.minBottomMargin)
        $0.bottom.equalTo(self.stackView).offset(UX.minBottomMargin)
      }
      stackView.snp.makeConstraints {
        $0.left.right.equalTo(self).inset(UX.textMargin)
      }
    }

    func updateContentInset() {
      stackView.layoutIfNeeded()
      if stackView.bounds.height < bounds.height {
        // Center it in the container
        scrollView.contentInset.top = ceil((scrollView.frame.height - stackView.bounds.height) / 2.0)
      } else {
        scrollView.contentInset.top = 0
      }
    }

    override func layoutSubviews() {
      super.layoutSubviews()
      updateContentInset()
    }

    required init?(coder aDecoder: NSCoder) {
      fatalError("init(coder:) has not been implemented")
    }
  }
}
