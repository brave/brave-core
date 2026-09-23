// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// A 3 part option for shield levels varying in strength of blocking content
public enum HTTPSUpgradeLevel: String, CaseIterable, Hashable {
  /// Always upgrade websites and block websites that cannot be upgraded
  case strict
  /// Always upgrade websites but allow http when it cannot be upgraded
  case standard
  /// Mode indicating this setting is disabled
  case disabled

  /// Wether this setting indicates that the shields are enabled or not
  public var isEnabled: Bool {
    switch self {
    case .strict, .standard: return true
    case .disabled: return false
    }
  }

  /// Wether this setting indicates that the shields are aggressive or not
  public var isStrict: Bool {
    switch self {
    case .strict: return true
    case .disabled, .standard: return false
    }
  }
}
