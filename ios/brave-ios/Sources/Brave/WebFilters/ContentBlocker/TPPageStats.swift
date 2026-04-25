// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

struct TPPageStats {
  let adCount: Int
  let trackerCount: Int
  let scriptCount: Int
  let fingerprintingCount: Int
  let httpsCount: Int

  var total: Int { return adCount + trackerCount + scriptCount + fingerprintingCount + httpsCount }

  init(
    adCount: Int = 0,
    trackerCount: Int = 0,
    scriptCount: Int = 0,
    fingerprintingCount: Int = 0,
    httpsCount: Int = 0
  ) {
    self.adCount = adCount
    self.trackerCount = trackerCount
    self.scriptCount = scriptCount
    self.fingerprintingCount = fingerprintingCount
    self.httpsCount = httpsCount
  }

  func adding(
    adCount: Int = 0,
    trackerCount: Int = 0,
    scriptCount: Int = 0,
    fingerprintingCount: Int = 0,
    httpsCount: Int = 0
  ) -> TPPageStats {
    TPPageStats(
      adCount: self.adCount + adCount,
      trackerCount: self.trackerCount + trackerCount,
      scriptCount: self.scriptCount + scriptCount,
      fingerprintingCount: self.fingerprintingCount + fingerprintingCount,
      httpsCount: self.httpsCount + httpsCount
    )
  }
}
