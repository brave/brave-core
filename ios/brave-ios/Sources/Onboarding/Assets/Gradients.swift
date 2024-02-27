// swift-format-ignore-file

// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import UIKit

enum OnboardingColor: String {
  case background_gradient_step0 = "background-gradient-step0"
  case background_gradient_step1 = "background-gradient-step1"

  var color: UIColor {
    return UIColor(named: rawValue, in: .module, compatibleWith: nil)!
  }
}

extension BraveGradient {

  static var backgroundGradient: BraveGradient {
    .init(
      stops: [
        .init(color: OnboardingColor.background_gradient_step0.color, position: 0.17),
        .init(color: OnboardingColor.background_gradient_step1.color, position: 1.0),
      ],
      angle: .figmaDegrees(138)
    )
  }
}
