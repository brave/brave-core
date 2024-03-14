// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import UIKit

enum FocusOnboarding {
  static let sliderBackground = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return .white
    } else {
      return UIColor(rgb: 0x0D1214)
    }
  })
}
