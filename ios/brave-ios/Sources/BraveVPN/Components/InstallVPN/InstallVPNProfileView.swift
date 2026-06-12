// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import GuardianConnect
import Lottie
import SwiftUI

public struct InstallVPNProfileView: View {
  @Environment(\.dismiss) private var dismiss

  private var connectToVPN: @Sendable () async -> Bool

  @State private var isLoading: Bool = false
  @State private var isConnectionErrorPresented: Bool = false

  public init() {
    self.connectToVPN = {
      await withCheckedContinuation { continuation in
        BraveVPN.connectToVPN { success in
          continuation.resume(returning: success)
        }
      }
    }
  }

  // To allow mocking
  init(connectToVPN: @escaping @Sendable () async -> Bool) {
    self.connectToVPN = connectToVPN
  }

  public var body: some View {
    NavigationStack {
      ScrollView {
        VStack {
          Image("install_vpn_image", bundle: .module)
            .resizable()
            .aspectRatio(contentMode: .fit)
            .clipShape(.rect(cornerRadius: 20, style: .continuous))
            .padding(.horizontal)
          VStack(alignment: .leading, spacing: 8) {
            Text(Strings.VPN.installProfileTitle)
              .font(.title3.weight(.semibold))
            Text(Strings.VPN.installProfileBody)
          }
          .padding(24)
        }
      }
      .osAvailabilityModifiers { content in
        if #available(iOS 26.0, *), LiquidGlassMode.isEnabled {
          content.safeAreaBar(edge: .bottom, spacing: 0) {
            actions
          }
        } else {
          content.safeAreaInset(edge: .bottom, spacing: 0) {
            actions
              .background(.background)
          }
        }
      }
      .navigationTitle(Strings.VPN.installTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          if #available(iOS 26.0, *) {
            Button(role: .close) {
              dismiss()
            }
          } else {
            Button(Strings.CancelString) {
              dismiss()
            }
          }
        }
      }
      .alert(
        Strings.VPN.vpnConfigGenericErrorTitle,
        isPresented: $isConnectionErrorPresented
      ) {
        Button(Strings.OKString) {}
      } message: {
        Text(Strings.VPN.vpnConfigGenericErrorBody)
      }

    }
  }

  private var actions: some View {
    VStack(spacing: 16) {
      Button {
        Task {
          await installVPNProfile()
        }
      } label: {
        Text(Strings.VPN.installProfileButtonText)
          .frame(maxWidth: .infinity)
          .opacity(isLoading ? 0 : 1)
          .overlay {
            if isLoading {
              ProgressView()
            }
          }
      }
      .disabled(isLoading)
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
      NavigationLink {
        VPNContactFormView()
      } label: {
        Text(Strings.VPN.settingsContactSupport)
          .font(.callout.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .schemesOnSurfaceVariant))
          .frame(maxWidth: .infinity)
      }
    }
    .padding(.horizontal)
    .padding(.bottom)  // Don't need top padding
  }

  private func installVPNProfile() async {
    isLoading = true
    defer { isLoading = false }

    // Used to set whether our current user is actively a paying customer
    // This has to be set before adding the profile otherwise it might fail
    GRDSubscriptionManager.setIsPayingUser(true)

    // Attempt to install & connect to the VPN retrying up to 2 times
    var numberOfRetries = 2
    var isConnected: Bool = false
    repeat {
      isConnected = await connectToVPN()
      if !isConnected {
        numberOfRetries -= 1
      }
    } while !isConnected && numberOfRetries > 0

    // If still not connected present an error
    if !isConnected {
      isConnectionErrorPresented = true
    } else {
      dismiss()
      showSuccessAlert()
    }
  }

  private func showSuccessAlert() {
    let animation = LottieAnimationView(name: "vpncheckmark", bundle: .module).then {
      $0.bounds = CGRect(x: 0, y: 0, width: 300, height: 200)
      $0.contentMode = .scaleAspectFill
      $0.play()
    }

    let popup = AlertPopupView(
      imageView: animation,
      title: Strings.VPN.installSuccessPopup,
      message: "",
      titleWeight: .semibold,
      titleSize: 18,
      dismissHandler: { true }
    )

    popup.showWithType(showType: .flyUp, autoDismissTime: 1.5)
  }
}

#if DEBUG

#Preview("Install Succeeds") {
  InstallVPNProfileView(
    connectToVPN: {
      try? await Task.sleep(for: .seconds(1))
      return true
    }
  )
}

#Preview("Install Fails") {
  InstallVPNProfileView(
    connectToVPN: {
      return false
    }
  )
}

#Preview("Presented") {
  Color.clear
    .sheet(isPresented: .constant(true)) {
      InstallVPNProfileView(
        connectToVPN: { return true }
      )
    }
}

#endif
