/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveCore

struct MultipleNetworkIconsView: View {
  let networks: [BraveWallet.NetworkInfo]
  let maxIcons = 3
  @ScaledMetric var iconSize = 16.0
  var maxIconSize: CGFloat = 32
  @ScaledMetric var iconDotSize = 2.0

  var body: some View {
    MultipleCircleIconView(
      models: networks,
      iconSize: iconSize,
      maxIconSize: maxIconSize,
      iconDotSize: iconDotSize,
      iconView: { network in
        NetworkIcon(network: network)
      }
    )
  }
}
