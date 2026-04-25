// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

public typealias Timestamp = UInt64
public typealias MicrosecondTimestamp = UInt64

extension Date {
  public static func now() -> Timestamp {
    return UInt64(1000 * Date().timeIntervalSince1970)
  }

  public static func nowMicroseconds() -> MicrosecondTimestamp {
    return UInt64(1_000_000 * Date().timeIntervalSince1970)
  }
}
