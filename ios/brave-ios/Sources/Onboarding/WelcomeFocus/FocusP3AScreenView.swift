// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Growth
import Preferences
import SafariServices
import SwiftUI

struct FocusP3AScreenView: View {
  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.verticalSizeClass) private var verticalSizeClass: UserInterfaceSizeClass?
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass: UserInterfaceSizeClass?

  @State private var isP3AHelpPresented = false
  @State private var isSystemSettingsViewPresented = false

  @Binding var shouldDismiss: Bool

  private let attributionManager: AttributionManager?
  private let p3aUtilities: BraveP3AUtils?

  private var shouldUseExtendedDesign: Bool {
    return horizontalSizeClass == .regular && verticalSizeClass == .regular
  }

  private let dynamicTypeRange = (...DynamicTypeSize.large)

  init(
    attributionManager: AttributionManager? = nil,
    p3aUtilities: BraveP3AUtils? = nil,
    shouldDismiss: Binding<Bool>
  ) {
    self.attributionManager = attributionManager
    self.p3aUtilities = p3aUtilities
    self._shouldDismiss = shouldDismiss
  }

  var body: some View {
    if shouldUseExtendedDesign {
      VStack(spacing: 40) {
        consentp3aContentView
          .background(colorScheme == .dark ? .black : .white)
          .clipShape(RoundedRectangle(cornerRadius: 16.0, style: .continuous))
          .frame(maxWidth: 616, maxHeight: 895)
          .shadow(color: .black.opacity(0.1), radius: 18, x: 0, y: 8)
          .shadow(color: .black.opacity(0.05), radius: 0, x: 0, y: 1)
        FocusStepsPagingIndicator(totalPages: 4, activeIndex: .constant(2))
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .background(Color(braveSystemName: .pageBackground))
      .background {
        NavigationLink("", isActive: $isSystemSettingsViewPresented) {
          FocusSystemSettingsView(screenType: .onboarding, shouldDismiss: $shouldDismiss)
        }
      }
      .toolbar(.hidden, for: .navigationBar)
    } else {
      VStack {
        consentp3aContentView
        FocusStepsPagingIndicator(totalPages: 4, activeIndex: .constant(2))
          .padding(.bottom, 20)
      }
      .background(Color(braveSystemName: .pageBackground))
      .background {
        NavigationLink("", isActive: $isSystemSettingsViewPresented) {
          FocusSystemSettingsView(screenType: .onboarding, shouldDismiss: $shouldDismiss)
        }
      }
      .toolbar(.hidden, for: .navigationBar)
    }
  }

  private var consentp3aContentView: some View {
    VStack {
      Image(
        shouldUseExtendedDesign ? "focus-product-insight-largescreen" : "focus-product-insight",
        bundle: .module
      )
      .resizable()
      .frame(
        maxWidth: shouldUseExtendedDesign ? 198 : 172,
        maxHeight: shouldUseExtendedDesign ? 242 : 204
      )
      .aspectRatio(contentMode: .fit)
      .padding(.bottom, 20)

      VStack {
        VStack(spacing: 8) {
          Text(Strings.FocusOnboarding.p3aScreenTitle)
            .font(
              Font.custom("Poppins-SemiBold", size: 28)
            )
            .opacity(0.9)
          Text(Strings.FocusOnboarding.p3aScreenDescription)
            .font(
              Font.custom("Poppins-Medium", size: 17)
            )
            .multilineTextAlignment(.center)
            .foregroundColor(Color(braveSystemName: .textTertiary))
        }
        .fixedSize(horizontal: false, vertical: true)
        .dynamicTypeSize(dynamicTypeRange)
        .padding(.bottom, shouldUseExtendedDesign ? 46 : 16)

        Toggle(
          isOn: Binding(
            get: { p3aUtilities?.isP3AEnabled ?? true },
            set: { newValue in p3aUtilities?.isP3AEnabled = newValue }
          )
        ) {
          VStack(alignment: .leading, spacing: 8) {
            Text(Strings.FocusOnboarding.p3aToggleTitle)
              .font(
                Font.custom("Poppins-Medium", size: 17)
              )
              .foregroundColor(Color(braveSystemName: .textPrimary))
              .opacity(0.9)
            Text(Strings.FocusOnboarding.p3aToggleDescription)
              .font(
                Font.custom("Poppins-Regular", size: 13)
              )
              .foregroundColor(Color(braveSystemName: .textTertiary))
          }
          .fixedSize(horizontal: false, vertical: true)
          .dynamicTypeSize(dynamicTypeRange)
          .padding(.vertical, 16)
          .padding(.horizontal, 20)
        }
        .padding(.bottom, shouldUseExtendedDesign ? 20 : 16)
        .padding(.trailing, 36)
        .toggleStyle(SwitchToggleStyle(tint: Color(braveSystemName: .buttonBackground)))
        Button(
          action: {
            isP3AHelpPresented = true
          },
          label: {
            Text(Strings.FocusOnboarding.p3aInformationButtonTitle)
              .font(
                Font.custom("Poppins-Regular", size: 13)
              )
              .foregroundColor(Color(braveSystemName: .textInteractive))
              .fixedSize(horizontal: false, vertical: true)
              .dynamicTypeSize(dynamicTypeRange)
              .multilineTextAlignment(.center)
          }
        )
        .padding(.horizontal, 20)
        .padding(.bottom, 8)
        .sheet(isPresented: $isP3AHelpPresented) {
          FocusSafariControllerView(url: FocusOnboardingConstants.p3aHelpArticle)
        }
      }
      .padding(.horizontal, shouldUseExtendedDesign ? 72 : 12)

      Spacer()

      Button(
        action: {
          handleAdCampaignLookupAndDAUPing(isP3AEnabled: p3aUtilities?.isP3AEnabled ?? false)

          isSystemSettingsViewPresented = true
        },
        label: {
          Text(Strings.FocusOnboarding.continueButtonTitle)
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
    .padding(.vertical, shouldUseExtendedDesign ? 64 : 20)
    .padding(.horizontal, shouldUseExtendedDesign ? 60 : 20)
  }

  private func handleAdCampaignLookupAndDAUPing(isP3AEnabled: Bool) {
    attributionManager?.pingDAUServer(isP3AEnabled)

    p3aUtilities?.isNoticeAcknowledged = true
    Preferences.Onboarding.p3aOnboardingShown.value = true
  }
}

private struct FocusSafariControllerView: UIViewControllerRepresentable {
  let url: URL

  func makeUIViewController(context: Context) -> SFSafariViewController {
    return SFSafariViewController(url: url)
  }

  func updateUIViewController(_ uiViewController: SFSafariViewController, context: Context) {}
}

#if DEBUG
struct FocusP3AScreenView_Previews: PreviewProvider {
  static var previews: some View {
    @State var shouldDismiss: Bool = false

    FocusP3AScreenView(shouldDismiss: $shouldDismiss)
  }
}
#endif
