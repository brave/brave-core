// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct BraveVPNPoweredBrandView: View {
  @State var isFreeTrialAvailable: Bool

  var body: some View {
    VStack(spacing: 8) {
      HStack(spacing: 5) {
        Text("Powered by")
          .font(.subheadline)
          .foregroundStyle(Color.white)
        Image(sharedName: "vpn_brand")
      }

      if isFreeTrialAvailable {
        HStack(alignment: .firstTextBaseline) {
          Text("All plans include a")
            .font(.subheadline)
            .foregroundStyle(Color.white)
          Text("free 7-day trial!")
            .font(.subheadline.weight(.semibold))
            .foregroundStyle(Color.white)
            .underline()
        }
      }
    }
    .padding()
    .frame(maxWidth: .infinity, alignment: .center)
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
