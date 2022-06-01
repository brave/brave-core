/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * Shared module is to be as unmodified as possible by Brave.
 *
 * This file is more of a catch-all for adding functionality that would traditionally be added into the Shared framework.
 * This allows easy merging at a later point, or even the ability to abstract to a separate framework.
 */

import Foundation
import Shared

extension Date {
  public func toTimestamp() -> Timestamp {
    return UInt64(self.timeIntervalSince1970 * 1000)
  }
}

extension Timestamp {
  public func toDate() -> Date {
    return Date(timeIntervalSince1970: TimeInterval(self / 1_000_000))
  }
}
