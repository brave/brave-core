// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct BraveVPNPoweredBrandView: View {
  @State var isFreeTrialAvailable: Bool

  var body: some View {
    VStack(alignment: UIDevice.current.orientation.isLandscape ? .leading : .center, spacing: 8) {
      HStack(spacing: 5) {
        Text(Strings.VPN.poweredBy)
        Image(sharedName: "vpn_brand")
      }

      if isFreeTrialAvailable {
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
    @State var isNoAvailable: Bool = false
    @State var isAvailable: Bool = true

    VStack {
      BraveVPNPoweredBrandView(isFreeTrialAvailable: isNoAvailable)

      Color(braveSystemName: .primitivePrimary25)
        .frame(height: 1.0)

      BraveVPNPoweredBrandView(isFreeTrialAvailable: isAvailable)
    }
    .background(
      Color(braveSystemName: .primitivePrimary10)
    )
  }
}
#endif
