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
    VStack(spacing: 0) {
      VStack(spacing: 16) {
        Text(Strings.VPN.vpnName)
          .font(.title2).fontWeight(.semibold)
        Text(Strings.VPN.settingHeaderBody)
          .font(.subheadline)
        Button(
          action: {
            enableVPNTapped?()
          },
          label: {
            HStack {
              Text(buttonTitle)
                .font(.body.weight(.semibold))
                .foregroundColor(Color(.white))
                .padding()
                .frame(maxWidth: .infinity)
            }
            .background(
              LinearGradient(
                gradient:
                  Gradient(colors: [
                    Color(UIColor(rgb: 0xFF4000)),
                    Color(UIColor(rgb: 0xFF1F01)),
                  ]),
                startPoint: .init(x: 0.26, y: 0.0),
                endPoint: .init(x: 0.26, y: 1.0)
              )
            )
          }
        )
        .clipShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
        HStack(spacing: 5) {
          Text(Strings.VPN.poweredBy)
          Image(sharedName: "vpn_brand")
        }
      }
      .padding(.init(top: 30, leading: 24, bottom: 24, trailing: 24))
      .foregroundColor(.white)
      .multilineTextAlignment(.center)
      .background(
        ZStack(alignment: .top) {
          Color(braveSystemName: .primitiveBlurple10)
          Image("enable_vpn_settings_banner_v2", bundle: .module)
            .scaledToFill()
        }
      )
      .clipShape(RoundedRectangle(cornerRadius: 16.0, style: .continuous))
      .overlay(alignment: .topTrailing) {
        Button {
          dismissHeaderTapped?()
        } label: {
          Image(braveSystemName: "leo.close")
            .font(.headline)
            .foregroundColor(.white.opacity(0.75))
            .padding(8)
        }
        .background(.ultraThinMaterial, in: Circle())
        .padding(.top, 10)
        .padding(.trailing, 12)
      }
    }
    .padding(.top, 20)
  }
}
