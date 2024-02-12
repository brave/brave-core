// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import SwiftUI
import BraveUI
import BraveVPN

/// A menu button that provides a shortcut to toggling Brave VPN
struct VPNMenuButton: View {
  /// The status indicating VPN is in retry state
  var retryStateActive: Bool
  /// The product info
  var vpnProductInfo: VPNProductInfo
  /// The description for product info
  var description: String?
  /// A closure executed when the parent must display a VPN-specific view controller due to some
  /// user action
  var displayVPNDestination: (UIViewController) -> Void
  /// A closure executed when VPN is toggled and status is installed. This will be used to set
  /// current activity for user
  var enableInstalledVPN: () -> Void
  /// A closure executed when the parent must display some alert
  var displayAlert: (UIAlertController) -> Void
  /// A closure to open a URL.
  var openURL: (URL) -> Void

  @State private var isVPNStatusChanging: Bool = BraveVPN.reconnectPending
  @State private var isVPNEnabled = BraveVPN.isConnected
  @State private var isErrorShowing: Bool = false
  
  @ScaledMetric private var iconSize: CGFloat = 32.0

  private var isVPNEnabledBinding: Binding<Bool> {
    Binding(
      get: { isVPNEnabled },
      set: { toggleVPN($0) }
    )
  }

  private func toggleVPN(_ enabled: Bool) {
    if BraveSkusManager.keepShowingSessionExpiredState {
      let alert = BraveSkusManager.sessionExpiredStateAlert(loginCallback: { _ in
        openURL(.brave.account)
      })
      
      displayAlert(alert)
      return
    }
    
    let vpnState = BraveVPN.vpnState

    if !VPNProductInfo.isComplete {
      isErrorShowing = true
      // Reattempt to connect to the App Store to get VPN prices.
      vpnProductInfo.load()
      return
    }
    switch BraveVPN.vpnState {
    case .notPurchased, .expired:
      // Expired Subcriptions can cause glitch because of connect on demand
      // Disconnect VPN before showing Purchase
      BraveVPN.disconnect(skipChecks: true)

      guard let vc = vpnState.enableVPNDestinationVC else { return }
      displayVPNDestination(vc)
    case .purchased:
      isVPNStatusChanging = true
      // Do not modify UISwitch state here, update it based on vpn status observer.
      if enabled {
        BraveVPN.reconnect()
        enableInstalledVPN()
      } else {
        BraveVPN.disconnect()
      }
    }
  }

  private var vpnToggle: some View {
    Toggle("Brave VPN", isOn: isVPNEnabledBinding)
      .toggleStyle(SwitchToggleStyle(tint: retryStateActive ? Color(.braveErrorBorder) : .accentColor))
  }

  var body: some View {
    HStack {
      headerView
      Spacer()
      if isVPNStatusChanging {
        ActivityIndicatorView(isAnimating: true)
      }
      vpnToggle
        .labelsHidden()
    }
    .padding(.horizontal, 14)
    .frame(maxWidth: .infinity, minHeight: 48.0, alignment: .leading)
    .background(
      Button(action: { toggleVPN(!BraveVPN.isConnected) }) {
        Color.clear
      }
      .buttonStyle(TableCellButtonStyle())
    )
    .accessibilityElement()
    .accessibility(addTraits: .isButton)
    .accessibility(label: Text("Brave VPN"))
    .alert(isPresented: $isErrorShowing) {
      Alert(
        title: Text(verbatim: Strings.VPN.errorCantGetPricesTitle),
        message: Text(verbatim: Strings.VPN.errorCantGetPricesBody),
        dismissButton: .default(Text(verbatim: Strings.OKString))
      )
    }
    .onReceive(NotificationCenter.default.publisher(for: .NEVPNStatusDidChange)) { _ in
      isVPNEnabled = BraveVPN.isConnected
      
      if BraveVPN.isConnected {
        isVPNStatusChanging = false
      } else {
        isVPNStatusChanging = BraveVPN.reconnectPending
      }
    }
  }
  
  private var headerView: some View {
    HStack(spacing: 14) {
      Image(braveSystemName: retryStateActive ? "leo.warning.triangle-filled" : "leo.product.vpn")
        .font(.body)
        .frame(width: iconSize, height: iconSize)
        .foregroundColor(retryStateActive ? Color(.braveErrorLabel) : Color(.braveLabel))
        .background(
          RoundedRectangle(cornerRadius: 8, style: .continuous)
            .fill(Color(.secondaryBraveGroupedBackground))
            .shadow(color: Color.black.opacity(0.1), radius: 1, x: 0, y: 1)
        )
        .padding(.vertical, 2)
      VStack(alignment: .leading, spacing: 3) {
        Text(verbatim: description == nil ? "Brave VPN" : Strings.OptionsMenu.braveVPNItemTitle)
        if let subTitle = description {
          Text(retryStateActive ? Strings.VPN.vpnUpdatePaymentMethodDescriptionText : subTitle)
            .font(.subheadline)
            .foregroundColor(retryStateActive ? Color(.braveErrorLabel) : Color(.secondaryBraveLabel))
        }
      }
      .padding(.vertical, description != nil ? 5 : 0)
    }
    .foregroundColor(Color(.braveLabel))
  }
}
