// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A 3 part option for shield levels varying in strength of blocking content
public enum ShieldLevel: String, CaseIterable, Hashable {
  /// Mode blocks all content
  case aggressive
  /// Mode indicating that 1st party content is not blocked for default and regional lists
  case standard
  /// Mode indicating this setting is disabled
  case disabled
  
  /// Wether this setting indicates that the shields are enabled or not
  public var isEnabled: Bool {
    switch self {
    case .aggressive, .standard: return true
    case .disabled: return false
    }
  }
  
  /// Wether this setting indicates that the shields are aggressive or not
  public var isAggressive: Bool {
    switch self {
    case .aggressive: return true
    case .disabled, .standard: return false
    }
  }
}
