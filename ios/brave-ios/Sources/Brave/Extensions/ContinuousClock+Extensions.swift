// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension ContinuousClock.Instant {
  func formatted(since: ContinuousClock.Instant) -> String {
    let duration = self - since
    return duration.formatted(
      .units(allowed: [
        .seconds, .milliseconds,
      ])
    )
  }

  func formattedUnits(since: ContinuousClock.Instant) -> String {
    let duration = self - since
    let formated = duration.formatted(.units())
    return formated
  }

  func formatted<S>(since: ContinuousClock.Instant, format: S) -> S.FormatOutput
  where S: FormatStyle, S.FormatInput == Duration {
    let duration = self - since
    return duration.formatted(format)
  }
}
