// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

extension Int {

  /// Return number of minutes in seconds.
  public var minutes: TimeInterval {
    return TimeInterval(self * 60)
  }

  /// Returns number of hours in seconds.
  public var hours: TimeInterval {
    return TimeInterval(self.minutes * 60)
  }

  /// Returns number of days in seconds.
  public var days: TimeInterval {
    return TimeInterval(self.hours * 24)
  }
}
