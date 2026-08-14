// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import Foundation

extension HTTPSUpgradeLevel {
  /// The value stored in the `kHttpsUpgradeLevel` preference, matching
  /// `brave_shields::HttpsUpgradeLevel`
  var prefValue: Int {
    switch self {
    case .disabled: return 0
    case .standard: return 1
    case .strict: return 2
    }
  }

  init?(prefValue: Int) {
    switch prefValue {
    case 0: self = .disabled
    case 1: self = .standard
    case 2: self = .strict
    default: return nil
    }
  }
}
