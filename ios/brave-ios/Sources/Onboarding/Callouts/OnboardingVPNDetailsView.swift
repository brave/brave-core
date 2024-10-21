// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveStrings
import BraveUI
import Shared
import SwiftUI

public struct OnboardingVPNDetailsView: View {
  @Environment(\.presentationMode) @Binding private var presentationMode

  private let dynamicTypeRange = (...DynamicTypeSize.xLarge)

  public var learnMore: (() -> Void)?

  private let descriptionItems = [
    Strings.VPN.infoCheckPrivacy,
    Strings.VPN.infoCheckLocation,
    Strings.VPN.infoCheckServers,
    Strings.VPN.infoCheckConnectionSpeed,
    Strings.VPN.infoCheckLimitDevice,
  ]

  public init() {}

  public var body: some View {
    VStack(spacing: 16) {
      VStack(spacing: 8) {
        Image("vpn_popup_shield", bundle: .module)
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
      .padding(.bottom, 8)
      Button {
        learnMore?()
      } label: {
        Text(Strings.learnMore)
          .font(.title3.weight(.medium))
          .padding(EdgeInsets(top: 12, leading: 24, bottom: 12, trailing: 24))
      }
      .background(Color(.braveBlurpleTint))
      .accentColor(Color(.white))
      .clipShape(Capsule())
    }
    .frame(maxWidth: BraveUX.baseDimensionValue)
    .padding(EdgeInsets(top: 18, leading: 24, bottom: 36, trailing: 24))
    .background(Color(.braveBackground))
    .dynamicTypeSize(dynamicTypeRange)
    .overlay {
      Button {
        presentationMode.dismiss()
      } label: {
        Image(braveSystemName: "leo.close")
          .renderingMode(.template)
          .foregroundColor(Color(.bravePrimary))
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topTrailing)
      .padding([.top, .trailing], 20)
      .dynamicTypeSize(dynamicTypeRange)
    }
  }
}

#if DEBUG
struct OnboardingVPNDetailsView_Previews: PreviewProvider {
  static var previews: some View {
    BraveUI.PopupView {
      OnboardingVPNDetailsView()
    }
  }
}
#endif
