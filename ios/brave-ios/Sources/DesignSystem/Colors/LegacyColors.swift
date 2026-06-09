// swift-format-ignore-file

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

enum LegacyDesignSystemColor: String {
  case gradient01_step0 = "gradient01-step0"
  case gradient01_step1 = "gradient01-step1"

  case gradient02_step0 = "gradient02-step0"
  case gradient02_step1 = "gradient02-step1"
  case gradient02_step2 = "gradient02-step2"

  case gradient05_step0 = "gradient05-step0"
  case gradient05_step1 = "gradient05-step1"

  var color: UIColor {
    return UIColor(named: rawValue, in: .module, compatibleWith: nil)!
  }
}

