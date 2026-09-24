// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Preferences
import SwiftUI

struct BraveVPNPoweredBrandView: View {
  @ObservedObject private var freeTrialUsed = Preferences.VPN.freeTrialUsed
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @Environment(\.verticalSizeClass) private var verticalSizeClass

  var body: some View {
    VStack(
      alignment: (horizontalSizeClass == .compact && verticalSizeClass == .compact)
        ? .leading : .center,
      spacing: 8
    ) {
      HStack(spacing: 5) {
        Text(Strings.VPN.poweredBy)
        Image(sharedName: "vpn_brand")
      }

      if !freeTrialUsed.value {
        Text("\(Strings.VPN.freeTrialDetail) ")
          + Text("\(Strings.VPN.freeTrialPeriod)!")
          .underline()
          .fontWeight(.medium)
      }
    }
    .font(.subheadline)
    .foregroundColor(.white)
  }
}

#if DEBUG
struct BraveVPNPoweredBrandView_Previews: PreviewProvider {
  static var previews: some View {
    VStack {
      BraveVPNPoweredBrandView()
    }
    .background(
      Color(braveSystemName: .primitivePrimary10)
    )
  }
}
#endif
