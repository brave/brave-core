// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import SwiftUI
import BraveUI

/// A menu button that provides a shortcut to toggling Brave VPN
struct VPNMenuButton: View {
    /// The product info
    var vpnProductInfo: VPNProductInfo
    /// A closure executed when the parent must display a VPN-specific view controller due to some
    /// user action
    var displayVPNDestination: (UIViewController) -> Void
    /// A closure executed when VPN is toggled and status is installed. This will be used to set
    /// current activity for user
    var enableInstalledVPN: () -> Void
    
    @State private var isVPNStatusChanging: Bool = BraveVPN.reconnectPending
    @State private var isVPNEnabled = BraveVPN.isConnected
    @State private var isErrorShowing: Bool = false
    
    private var isVPNEnabledBinding: Binding<Bool> {
        Binding(
            get: { isVPNEnabled },
            set: { toggleVPN($0) }
        )
    }
    
    private func toggleVPN(_ enabled: Bool) {
        let vpnState = BraveVPN.vpnState
        
        if !VPNProductInfo.isComplete {
            isErrorShowing = true
            // Reattempt to connect to the App Store to get VPN prices.
            vpnProductInfo.load()
            return
        }
        switch BraveVPN.vpnState {
        case .notPurchased, .purchased, .expired:
            guard let vc = vpnState.enableVPNDestinationVC else { return }
            displayVPNDestination(vc)
        case .installed:
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
        Group {
            let toggle = Toggle("Brave VPN", isOn: isVPNEnabledBinding)
            if #available(iOS 14.0, *) {
                toggle
                    .toggleStyle(SwitchToggleStyle(tint: .accentColor))
            } else {
                toggle
            }
        }
    }
    
    var body: some View {
        HStack {
            MenuItemHeaderView(icon: #imageLiteral(resourceName: "vpn_menu_icon").template, title: "Brave VPN")
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
            isVPNStatusChanging = BraveVPN.reconnectPending
        }
    }
}
