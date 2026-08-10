// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

public struct BraveVPNEnableSettingsHeaderView: View {

  public var enableVPNTapped: (() -> Void)?
  public var dismissHeaderTapped: (() -> Void)?

  private var buttonTitle: String {
    switch BraveVPN.vpnState {
    case .notPurchased:
      return Strings.VPN.tryForFreeButton
    case .expired:
      return Strings.learnMore
    case .purchased:
      return Strings.VPN.enableButton
    }
  }

  public init(
    enableVPNTapped: (() -> Void)? = nil,
    dismissHeaderTapped: (() -> Void)? = nil
  ) {
    self.enableVPNTapped = enableVPNTapped
    self.dismissHeaderTapped = dismissHeaderTapped
  }

  public var body: some View {
    VStack(spacing: 16) {
      Text(Strings.VPN.vpnName)
        .font(.title2.weight(.semibold))
      Text(Strings.VPN.settingHeaderBody)
        .font(.subheadline)
      Button {
        enableVPNTapped?()
      } label: {
        Text(buttonTitle)
          .frame(maxWidth: .infinity)
      }
      .controlSize(.extraLarge)
      .osAvailabilityModifiers { content in
        if #available(iOS 26.0, *) {
          content.buttonStyle(.glassHero)
        } else {
          content.buttonStyle(.hero)
        }
      }
      HStack(spacing: 5) {
        Text(Strings.VPN.poweredBy)
        Image(sharedName: "vpn_brand")
      }
      .accessibilityElement()
      .accessibilityLabel(Strings.VPN.poweredByGuardianAccessibilityLabel)
    }
    .padding(24)
    .foregroundStyle(.white)
    .multilineTextAlignment(.center)
    .overlay(alignment: .topTrailing) {
      Button {
        dismissHeaderTapped?()
      } label: {
        Label(Strings.close, braveSystemImage: "leo.close")
          .labelStyle(.iconOnly)
          .font(.headline)
          .foregroundStyle(Color.white.opacity(0.25))
      }
      .padding(16)
    }
    .background(
      LinearGradient(
        colors: [
          Color(braveSystemName: .primitiveNeutral15),
          Color(braveSystemName: .primitiveNeutral5),
        ],
        startPoint: .top,
        endPoint: .bottom
      )
      .shadow(.inner(color: Color(braveSystemName: .primitiveNeutral25), radius: 0, y: 1)),
      in: .rect(cornerRadius: 24, style: .continuous)
    )
  }
}

#if DEBUG
#Preview {
  BraveVPNEnableSettingsHeaderView()
    .padding()
}
#endif
