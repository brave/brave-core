// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Shared
import BraveUI
import BraveShared

public struct OnboardingVPNDetailsView: View {
  public var learnMore: (() -> Void)?
  
  private let descriptionItems = [Strings.VPN.popupCheckboxBlockAds,
                          Strings.VPN.popupCheckmarkSecureConnections,
                          Strings.VPN.popupCheckboxFast,
                          Strings.VPN.popupCheckmark247Support]

  public init() {}

  public var body: some View {
    VStack(spacing: 16) {
      VStack(spacing: 12) {
        Image("vpn_popup_shield", bundle: .module)
          .padding(.bottom)
          .accessibilityHidden(true)
        Text(Strings.VPN.vpnName)
          .font(.title.weight(.heavy))
          .foregroundColor(Color(.bravePrimary))
          .multilineTextAlignment(.center)
        HStack(spacing: 8) {
          Text(Strings.VPN.poweredBy)
            .font(.footnote)
            .foregroundColor(Color(.bravePrimary))
            .multilineTextAlignment(.center)
          Image(sharedName: "vpn_brand")
            .renderingMode(.template)
            .foregroundColor(Color(.bravePrimary))
        }
        .padding(.vertical, 12)
      }
      .frame(maxWidth: .infinity)
      VStack(alignment: .leading, spacing: 8) {
        ForEach(descriptionItems, id: \.self) { itemDescription in
          HStack(spacing: 8) {
            Image(sharedName: "vpn_checkmark_popup")
              .renderingMode(.template)
              .foregroundColor(Color(.bravePrimary))
              .frame(alignment: .leading)
            Text(itemDescription)
              .multilineTextAlignment(.leading)
              .foregroundColor(Color(.bravePrimary))
              .fixedSize(horizontal: false, vertical: true)
          }
        }
      }
      .frame(maxWidth: .infinity)
      .padding(.bottom, 8)
      Button(action: {
        learnMore?()
      }) {
        Text(Strings.learnMore)
          .font(.title3.weight(.medium))
          .padding(EdgeInsets(top: 12, leading: 24, bottom: 12, trailing: 24))
      }
      .background(Color(.braveBlurpleTint))
      .accentColor(Color(.white))
      .clipShape(Capsule())
    }
    .frame(maxWidth: BraveUX.baseDimensionValue)
    .padding(EdgeInsets(top: 24, leading: 24, bottom: 36, trailing: 24))
    .background(Color(.braveBackground))
    .accessibilityEmbedInScrollView()
  }
}

#if DEBUG
struct OnboardingVPNDetailsView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      BraveUI.PopupView {
        OnboardingVPNDetailsView()
      }
      .previewDevice("iPhone 12 Pro")

      BraveUI.PopupView {
        OnboardingVPNDetailsView()
      }
      .previewDevice("iPad Pro (9.7-inch)")
    }
  }
}
#endif
