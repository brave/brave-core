// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

/// A setting that will shred site data at various times
public enum SiteShredLevel: String, CaseIterable, Hashable {
  /// An explicit value to never shred site data
  case never
  /// Shred the site data when the site is closed
  case whenSiteClosed
  /// Shred the site data when the application is closed
  case appExit

  /// Tells us if this setting shreds data when the app is closed
  public var shredOnAppExit: Bool {
    switch self {
    case .never, .whenSiteClosed:
      return false
    case .appExit:
      return true
    }
  }

  public var autoShredMode: BraveShields.AutoShredMode {
    switch self {
    case .never:
      return .never
    case .whenSiteClosed:
      return .lastTabClosed
    case .appExit:
      return .appExit
    }
  }
}

extension BraveShields.AutoShredMode {
  public var siteShredLevel: SiteShredLevel {
    switch self {
    case .never:
      return .never
    case .lastTabClosed:
      return .whenSiteClosed
    case .appExit:
      return .appExit
    @unknown default:
      assertionFailure("Unexpected AutoShredMode")
      return .never
    }
  }
}
