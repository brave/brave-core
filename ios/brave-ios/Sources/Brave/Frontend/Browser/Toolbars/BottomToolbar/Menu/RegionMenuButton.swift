// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import BraveVPN
import Foundation
import GuardianConnect
import Shared
import SwiftUI

/// A menu button that provides a shortcut to changing Brave VPN region
struct RegionMenuButton: View {
  /// Boolean determining if current setting subtitle is going to be shown
  var settingTitleEnabled = true
  /// A closure executed when the region select is clicked
  var regionSelectAction: () -> Void

  @State private var isVPNEnabled = BraveVPN.isConnected
  @State private var vpnRegionInfo: GRDRegion? = BraveVPN.activatedRegion

  /// Subtitle generation according to menu selection
  private var subTitle: String? {
    guard settingTitleEnabled else {
      return nil
    }

    return vpnRegionInfo?.settingTitle
      ?? String(
        format: Strings.VPN.vpnRegionSelectorButtonSubTitle,
        Strings.VPN.regionPickerAutomaticModeCellText
      )
  }

  var body: some View {
    HStack {
      if isVPNEnabled {
        HStack {
          MenuItemHeaderView(
            icon: vpnRegionInfo?.regionFlag ?? Image(braveSystemName: "leo.globe"),
            title: Strings.VPN.vpnRegionSelectorButtonTitle,
            subtitle: subTitle
          )
          Spacer()
          Image(braveSystemName: "leo.arrow.small-right")
            .foregroundColor(Color(.secondaryBraveLabel))
        }
        .padding(.horizontal, 14)
        .frame(maxWidth: .infinity, minHeight: 48.0, alignment: .leading)
        .background(
          Button {
            regionSelectAction()
          } label: {
            Color.clear
          }
          .buttonStyle(TableCellButtonStyle())
        )
        .accessibilityElement()
        .accessibility(addTraits: .isButton)
        .accessibility(
          label: Text(Strings.VPN.vpnRegionSelectorButtonTitle)
        )
      }

    }
    .onReceive(NotificationCenter.default.publisher(for: .NEVPNStatusDidChange)) { _ in
      isVPNEnabled = BraveVPN.isConnected
      vpnRegionInfo = BraveVPN.activatedRegion
    }
  }
}
