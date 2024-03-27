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
  @State private var isP3AToggleOn = true
  @State private var isP3AHelpPresented = false
  @State private var isSystemSettingsViewPresented = false

  @Binding var shouldDismiss: Bool

  private let attributionManager: AttributionManager?
  private let p3aUtilities: BraveP3AUtils?

  let dynamicTypeRange = (...DynamicTypeSize.xLarge)

  public init(
    attributionManager: AttributionManager? = nil,
    p3aUtilities: BraveP3AUtils? = nil,
    shouldDismiss: Binding<Bool>
  ) {
    self.attributionManager = attributionManager
    self.p3aUtilities = p3aUtilities
    self._shouldDismiss = shouldDismiss
  }

  var body: some View {
    NavigationView {
      VStack {
        Image("focus-product-insight", bundle: .module)
          .resizable()
          .aspectRatio(contentMode: .fit)
          .padding(.bottom, 24)

        VStack {
          VStack(spacing: 8) {
            Text("Make Brave Better")
              .font(
                Font.custom("FlechaM-Medium", size: 32)
              )
              .dynamicTypeSize(dynamicTypeRange)
              .fixedSize(horizontal: false, vertical: true)
              .opacity(0.9)

            Text("Let us know which features you’re enjoying the most.")
              .font(
                Font.custom("Poppins-Medium", size: 17)
              )
              .dynamicTypeSize(dynamicTypeRange)
              .fixedSize(horizontal: false, vertical: true)
              .multilineTextAlignment(.center)
              .foregroundColor(Color(braveSystemName: .textTertiary))
          }
          .padding(.bottom, 16)

          Toggle(isOn: $isP3AToggleOn) {
            VStack(alignment: .leading, spacing: 4) {
              Text("Share Completely Private & Anonymous Product Insights.")
                .font(
                  Font.custom("Poppins-Medium", size: 17)
                )
                .dynamicTypeSize(dynamicTypeRange)
                .fixedSize(horizontal: false, vertical: true)
                .foregroundColor(Color(braveSystemName: .textPrimary))
                .padding(.bottom, 4)
                .opacity(0.9)
              Text(
                LocalizedStringKey(
                  "Change this at any time in Brave Settings under ‘Brave Shields and Privacy’."
                )
              )
              .font(
                Font.custom("Poppins-Regular", size: 13)
              )
              .dynamicTypeSize(dynamicTypeRange)
              .fixedSize(horizontal: false, vertical: true)
              .foregroundColor(Color(braveSystemName: .textTertiary))
            }
            .padding(16)
            .padding(.horizontal, 4)
          }
          .padding(.bottom, 16)
          .padding(.trailing, 20)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          .toggleStyle(SwitchToggleStyle(tint: Color(braveSystemName: .buttonBackground)))
          .onChange(of: isP3AToggleOn) { newValue in
            p3aUtilities?.isP3AEnabled = newValue
          }

          Text("Learn more about our Privacy Preserving Product Analytics (P3A)")
            .font(
              Font.custom("Poppins-Regular", size: 13)
            )
            .foregroundColor(Color(braveSystemName: .textInteractive))
            .fixedSize(horizontal: false, vertical: true)
            .dynamicTypeSize(dynamicTypeRange)
            .multilineTextAlignment(.center)
            .padding(.horizontal, 20)
            .padding(.bottom, 20)
            .onTapGesture {
              isP3AHelpPresented = true
            }
            .sheet(isPresented: $isP3AHelpPresented) {
              FocusSafariControllerView(url: FocusOnboardingConstants.p3aHelpArticle)
            }
        }
        .padding(.horizontal, 20)

        Spacer()

        VStack(spacing: 28) {
          Button(
            action: {
              handleAdCampaignLookupAndDAUPing(isP3AEnabled: p3aUtilities?.isP3AEnabled ?? false)

              isSystemSettingsViewPresented = true
            },
            label: {
              Text(Strings.FocusOnboarding.continueButtonTitle)
                .font(.body.weight(.semibold))
                .foregroundColor(Color(.white))
                .dynamicTypeSize(dynamicTypeRange)
                .padding()
                .foregroundStyle(.white)
                .frame(maxWidth: .infinity)
                .background(Color(braveSystemName: .buttonBackground))
            }
          )
          .clipShape(RoundedRectangle(cornerRadius: 12.0))
          .overlay(RoundedRectangle(cornerRadius: 12.0).strokeBorder(Color.black.opacity(0.2)))

          FocusStepsPagingIndicator(totalPages: 4, activeIndex: .constant(2))
        }
        .padding(.bottom, 20)
      }
      .padding(.horizontal, 20)
      .background(Color(braveSystemName: .pageBackground))
      .background {
        NavigationLink("", isActive: $isSystemSettingsViewPresented) {
          FocusSystemSettingsView(shouldDismiss: $shouldDismiss)
        }
      }
    }
    .overlay(alignment: .topTrailing) {
      Button(
        action: {
          handleAdCampaignLookupAndDAUPing(isP3AEnabled: false)

          shouldDismiss = true
        },
        label: {
          Image("focus-icon-close", bundle: .module)
            .resizable()
            .frame(width: 32, height: 32)
            .padding(.trailing, 24)
        }
      )
    }
    .navigationBarHidden(true)
  }

  private func handleAdCampaignLookupAndDAUPing(isP3AEnabled: Bool) {
    attributionManager?.pingDAUServer(isP3AEnabled)

    p3aUtilities?.isNoticeAcknowledged = true
    Preferences.Onboarding.p3aOnboardingShown.value = true
  }
}

struct FocusSafariControllerView: UIViewControllerRepresentable {
  let url: URL

  func makeUIViewController(context: Context) -> SFSafariViewController {
    let safariViewController = SFSafariViewController(url: url)
    return safariViewController
  }

  func updateUIViewController(_ uiViewController: SFSafariViewController, context: Context) {}
}

struct FocusP3AScreenView_Previews: PreviewProvider {
  static var previews: some View {
    @State var shouldDismiss: Bool = false

    FocusP3AScreenView(shouldDismiss: $shouldDismiss)
  }
}
