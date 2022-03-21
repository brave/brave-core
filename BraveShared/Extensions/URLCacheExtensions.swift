/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

extension URLCache {

  /// Setup default URLCache settings for Brave
  public func setupBraveDefaults() {
    memoryCapacity = 6 * 1024 * 1024  // 6 MB
    diskCapacity = 40 * 1024 * 1024
  }
}
