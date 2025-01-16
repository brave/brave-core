// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Lottie
import Preferences
import SwiftUI

public struct FocusSystemSettingsView: View {

  public enum DefaultBrowserScreenType {
    case onboarding, callout
  }

  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.dismiss) var dismiss
  @Environment(\.verticalSizeClass) private var verticalSizeClass: UserInterfaceSizeClass?
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass: UserInterfaceSizeClass?

  @Binding var shouldDismiss: Bool
  @Binding var isCompleted: Bool

  var screenType: DefaultBrowserScreenType

  private var shouldUseExtendedDesign: Bool {
    return horizontalSizeClass == .regular && verticalSizeClass == .regular
  }

  private let dynamicTypeRange = (...DynamicTypeSize.xLarge)
  var namespace: Namespace.ID

  public init(
    namespace: Namespace.ID,
    screenType: DefaultBrowserScreenType,
    isCompleted: Binding<Bool>,
    shouldDismiss: Binding<Bool>
  ) {
    self.namespace = namespace
    self.screenType = screenType
    self._shouldDismiss = shouldDismiss
    self._isCompleted = isCompleted
  }

  public var body: some View {
    ZStack {
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
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(braveSystemName: .pageBackground))
        .toolbar(.hidden, for: .navigationBar)
      } else {
        VStack(spacing: 16) {
          settingsSystemContentView
        }
        .padding(.bottom, 20)
        .background(Color(braveSystemName: .pageBackground))
        .toolbar(.hidden, for: .navigationBar)
      }
    }
  }

  private var settingsSystemContentView: some View {
    VStack {
      if shouldUseExtendedDesign {
        HStack {
          Image("focus-icon-brave", bundle: .module)
            .resizable()
            .matchedGeometryEffect(id: "icon", in: namespace)
            .frame(width: 78, height: 78)

          Image("focus-brave-watermark", bundle: .module)
            .resizable()
            .frame(width: 111, height: 31)
        }
      } else {
        Image("focus-icon-brave", bundle: .module)
          .resizable()
          .matchedGeometryEffect(id: "icon", in: namespace)
          .frame(width: 78, height: 78)
      }

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
          bundle: .module,
          subdirectory: "LottieAssets"
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

      HStack(spacing: 12) {

        Button(
          action: {
            completeDefaultBrowserInteraction()
          },
          label: {
            Text(Strings.FocusOnboarding.notNowActionButtonTitle)
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

        Button(
          action: {
            if let settingsUrl = URL(string: UIApplication.openSettingsURLString) {
              UIApplication.shared.open(settingsUrl)
            }

            completeDefaultBrowserInteraction()
          },
          label: {
            Text(Strings.FocusOnboarding.systemSettingsButtonTitle)
              .font(.body.weight(.semibold))
              .foregroundColor(Color(braveSystemName: .schemesOnPrimary))
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
      }
    }
    .padding(.vertical, shouldUseExtendedDesign ? 48 : 24)
    .padding(.horizontal, shouldUseExtendedDesign ? 75 : 20)
  }

  private func completeDefaultBrowserInteraction() {
    if screenType == .onboarding {
      isCompleted = true
    } else {
      dismiss()
    }
  }
}

#if DEBUG
struct FocusSystemSettingsView_Previews: PreviewProvider {
  static var previews: some View {
    @State var shouldDismiss: Bool = false
    @State var isCompleted: Bool = false
    @Namespace var namespace

    FocusSystemSettingsView(
      namespace: namespace,
      screenType: .onboarding,
      isCompleted: $isCompleted,
      shouldDismiss: $shouldDismiss
    )

    FocusSystemSettingsView(
      namespace: namespace,
      screenType: .callout,
      isCompleted: $isCompleted,
      shouldDismiss: $shouldDismiss
    )
  }
}
#endif
