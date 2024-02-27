// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct MultipleAccountBlockiesView: View {
  let accountAddresses: [String]
  let maxBlockies = 3
  @ScaledMetric var blockieSize = 16.0
  var maxBlockieSize: CGFloat = 32
  @ScaledMetric var blockieDotSize = 2.0

  var body: some View {
    MultipleCircleIconView(
      models: accountAddresses,
      shape: .rectangle,
      iconSize: blockieSize,
      maxIconSize: maxBlockieSize,
      iconDotSize: blockieDotSize,
      iconView: { address in
        Blockie(address: address)
      }
    )
  }
}
