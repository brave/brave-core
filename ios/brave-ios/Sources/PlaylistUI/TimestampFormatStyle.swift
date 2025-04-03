// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Formats a Duration dynamically based on if the value is over an hour.
struct TimestampFormatStyle: FormatStyle {
  func format(_ value: Duration) -> String {
    let pattern: Duration.TimeFormatStyle.Pattern
    if abs(value.components.seconds) >= (60 * 60) {
      pattern = .hourMinuteSecond
    } else {
      pattern = .minuteSecond
    }
    return value.formatted(.time(pattern: pattern))
  }
}

extension FormatStyle where Self == TimestampFormatStyle {
  static var timestamp: TimestampFormatStyle {
    .init()
  }
}
