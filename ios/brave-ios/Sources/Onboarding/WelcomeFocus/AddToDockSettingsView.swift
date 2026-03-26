// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Lottie
import Strings
import SwiftUI

/// Add-to-dock instructions shown from Settings (sheet), without the full onboarding flow.
public struct AddToDockSettingsView: View {
  private let onDismiss: () -> Void

  @Environment(\.colorScheme) private var colorScheme
  @ScaledMetric private var titleFontSize = 24

  public init(onDismiss: @escaping () -> Void) {
    self.onDismiss = onDismiss
  }

  public var body: some View {
    ZStack {
      Color(braveSystemName: .iosBrowserChromeBackgroundIos)
        .ignoresSafeArea()

      VStack(spacing: 16) {
        VStack(alignment: .center, spacing: 8) {
          Text(Strings.FocusOnboarding.addToDockScreenTitle)
            .font(.system(size: titleFontSize, weight: .semibold))
            .foregroundStyle(Color(braveSystemName: .textPrimary))
          Text(Strings.FocusOnboarding.addToDockScreenDescription)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
        .multilineTextAlignment(.center)
        .frame(maxWidth: .infinity)
        .padding(.horizontal, 20)
        .padding(.top, 20)

        LottieView(
          animation: .named(
            colorScheme == .dark
              ? "onboarding-add-to-dock-dark"
              : "onboarding-add-to-dock-light",
            bundle: .module
          )
        )
        .resizable()
        .playing(loopMode: .loop)
        .id(colorScheme)

        Button {
          onDismiss()
        } label: {
          Text(Strings.close)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .padding(20)
        .background { Color(braveSystemName: .iosBrowserChromeBackgroundIos) }
      }
    }
  }
}

#if DEBUG
#Preview {
  AddToDockSettingsView(onDismiss: {})
}
#endif
