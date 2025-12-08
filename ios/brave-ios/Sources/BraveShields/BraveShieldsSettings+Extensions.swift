// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension BraveShieldsSettings {
  /// Returns the `AutoShredMode` for the given url, optionally checking if
  /// Shields is disabled on any host that matches the domain pattern.
  /// For example, this could occur when Shields is disabled on `one.brave.com`,
  /// but enabled on `two.brave.com` and Auto Shred default is set to app exit.
  /// - parameter url: The URL to fetch the `AutoShredMode` for.
  /// - parameter considerAllShieldsOption: Flag to determine if we check if
  /// Shields is disabled on any host matching the domain pattern.
  /// - returns: The `AutoShredMode` for the given URL.
  public func autoShredMode(
    for url: URL?,
    considerAllShieldsOption: Bool
  ) -> BraveShields.AutoShredMode {
    if considerAllShieldsOption, isShieldsDisabledOnAnyHostMatchingDomain(of: url) {
      return .never
    }
    return autoShredMode(for: url)
  }
}
