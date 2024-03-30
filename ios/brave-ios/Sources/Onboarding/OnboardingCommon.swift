// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import UIKit

struct OnboardingCommon {
  struct UX {
    /// The onboarding screens are showing as a modal on iPads.
    static let preferredModalSize = CGSize(width: 375, height: 667)

    /// A negative spacing is needed to make rounded corners for details view visible.
    static let negativeSpacing = -16.0
    static let descriptionContentInset = 25.0
    static let linkColor: UIColor = .braveBlurpleTint
    static let animationContentInset = 50.0
    static let checkboxInsets = -44.0

    static let contentBackgroundColor = UIColor(rgb: 0x1E2029)
    static let secondaryButtonTextColor = UIColor(rgb: 0x84889C)
  }

  struct Views {
    static func primaryButton(text: String = Strings.OBContinueButton) -> UIButton {
      let button = RoundInterfaceButton().then {
        $0.setTitle(text, for: .normal)
        $0.backgroundColor = .braveBlurpleTint
        $0.contentEdgeInsets = UIEdgeInsets(top: 12, left: 25, bottom: 12, right: 25)
      }

      return button
    }

    static func secondaryButton(text: String = Strings.OBSkipButton) -> UIButton {
      let button = UIButton().then {
        $0.setTitle(text, for: .normal)
        let titleColor = UX.secondaryButtonTextColor
        $0.setTitleColor(titleColor, for: .normal)
      }

      return button
    }

    static func primaryText(_ text: String) -> UILabel {
      let label = UILabel().then {
        $0.text = text
        $0.font = UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.bold)
        $0.textAlignment = .center
        $0.textColor = .braveLabel
      }

      return label
    }

    static func secondaryText(_ text: String) -> UILabel {
      let label = UILabel().then {
        $0.text = text
        $0.font = UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.regular)
        $0.textAlignment = .center
        $0.numberOfLines = 0
        $0.textColor = .braveLabel
      }

      return label
    }
  }
}
