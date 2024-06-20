// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Lottie
import Preferences
import SwiftUI

struct FocusSystemSettingsView: View {
  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.verticalSizeClass) private var verticalSizeClass: UserInterfaceSizeClass?
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass: UserInterfaceSizeClass?

  @Binding var shouldDismiss: Bool

  private var shouldUseExtendedDesign: Bool {
    return horizontalSizeClass == .regular && verticalSizeClass == .regular
  }

  private let dynamicTypeRange = (...DynamicTypeSize.xLarge)

  var body: some View {
    if shouldUseExtendedDesign {
      VStack(spacing: 40) {
        VStack {
          settingsSystemContentView
            .background(colorScheme == .dark ? .black : .white)
        }
        .clipShape(RoundedRectangle(cornerRadius: 16.0, style: .continuous))
        .frame(maxWidth: 616, maxHeight: 895)
        .shadow(color: .black.opacity(0.1), radius: 18, x: 0, y: 8)
        .shadow(color: .black.opacity(0.05), radius: 0, x: 0, y: 1)

        FocusStepsPagingIndicator(totalPages: 4, activeIndex: .constant(3))
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .background(Color(braveSystemName: .pageBackground))
      .toolbar(.hidden, for: .navigationBar)
    } else {
      VStack(spacing: 16) {
        settingsSystemContentView
        FocusStepsPagingIndicator(totalPages: 4, activeIndex: .constant(3))
      }
      .padding(.bottom, 20)
      .background(Color(braveSystemName: .pageBackground))
      .toolbar(.hidden, for: .navigationBar)
    }
  }

  private var settingsSystemContentView: some View {
    VStack {
      VStack(spacing: 10) {
        Text(Strings.FocusOnboarding.defaultBrowserScreenTitle)
          .font(
            Font.custom("Poppins-SemiBold", size: 28)
          )
          .multilineTextAlignment(.center)
        Text(Strings.FocusOnboarding.defaultBrowserScreenDescription)
          .font(
            Font.custom("Poppins-Medium", size: 17)
          )
          .multilineTextAlignment(.center)
          .foregroundColor(Color(braveSystemName: .textTertiary))
      }
      .dynamicTypeSize(dynamicTypeRange)
      .fixedSize(horizontal: false, vertical: true)
      .padding(.bottom, 24)

      Spacer()

      LottieView(
        animation: .named(
          colorScheme == .dark ? "browser-default-dark" : "browser-default-light",
          bundle: .module
        )
      )
      .playing(loopMode: .loop)
      .resizable()
      .aspectRatio(contentMode: .fit)
      .background(Color(braveSystemName: .pageBackground))
      .clipShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
      .overlay(
        RoundedRectangle(cornerRadius: 12.0, style: .continuous)
          .stroke(Color(braveSystemName: .textTertiary), lineWidth: 1)
      )
      .padding(.bottom, 24)

      Spacer()

      VStack(spacing: 12) {
        Button(
          action: {
            if let settingsUrl = URL(string: UIApplication.openSettingsURLString) {
              UIApplication.shared.open(settingsUrl)
            }

            Preferences.FocusOnboarding.urlBarIndicatorShowBeShown.value = true
            shouldDismiss = true
          },
          label: {
            Text(Strings.FocusOnboarding.systemSettingsButtonTitle)
              .font(.body.weight(.semibold))
              .foregroundColor(Color(.white))
              .dynamicTypeSize(dynamicTypeRange)
              .padding()
              .frame(maxWidth: .infinity)
              .background(Color(braveSystemName: .buttonBackground))
          }
        )
        .clipShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
        .overlay(
          RoundedRectangle(cornerRadius: 12.0, style: .continuous).strokeBorder(
            Color.black.opacity(0.2)
          )
        )

        Button(
          action: {
            Preferences.FocusOnboarding.urlBarIndicatorShowBeShown.value = true
            shouldDismiss = true
          },
          label: {
            Text(
              "\(Strings.FocusOnboarding.startBrowseActionButtonTitle) \(Image(systemName: "arrow.right"))"
            )
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(braveSystemName: .textInteractive))
            .dynamicTypeSize(dynamicTypeRange)
            .padding()
            .frame(maxWidth: .infinity)
          }
        )
        .clipShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
        .overlay(
          RoundedRectangle(cornerRadius: 12.0)
            .strokeBorder(Color(braveSystemName: .dividerInteractive).opacity(0.6), lineWidth: 1.0)
        )
      }
    }
    .padding(.vertical, shouldUseExtendedDesign ? 48 : 24)
    .padding(.horizontal, shouldUseExtendedDesign ? 75 : 20)
  }
}

#if DEBUG
struct FocusSystemSettingsView_Previews: PreviewProvider {
  static var previews: some View {
    @State var shouldDismiss: Bool = false

    FocusSystemSettingsView(shouldDismiss: $shouldDismiss)
  }
}
#endif
