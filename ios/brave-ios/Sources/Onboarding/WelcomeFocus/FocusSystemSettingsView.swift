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
  @Environment(\.dismiss) private var dismiss

  @State private var shouldDismiss = false

  var body: some View {
    NavigationView {
      VStack {
        VStack(spacing: 10) {
          Text("Set Brave as your Default Browser")
            .font(
              Font.custom("FlechaM-Medium", size: 32)
            )
            .lineLimit(2)
            .multilineTextAlignment(.center)
            .fixedSize(horizontal: false, vertical: true)

          Text("Open every link you tap with Brave’s privacy protections")
            .font(
              Font.custom("Poppins-Medium", size: 17)
            )
            .lineLimit(2)
            .multilineTextAlignment(.center)
            .fixedSize(horizontal: false, vertical: true)
            .foregroundColor(Color(braveSystemName: .textTertiary))
        }
        .padding(.top, 25)
        .padding(.vertical, 16)

        LottieAnimationView(
          name: colorScheme == .dark ? "browser-default-dark" : "browser-default-light",
          bundle: .module
        )
        .loopMode(.loop)
        .resizable()
        .frame(height: 364)
        .aspectRatio(contentMode: .fill)
        .clipShape(RoundedRectangle(cornerRadius: 12.0))
        .overlay(
          RoundedRectangle(cornerRadius: 12.0)
            .stroke(Color(braveSystemName: .textTertiary), lineWidth: 1)
        )

        Spacer()

        VStack(spacing: 24) {
          Button(
            action: {
              if let settingsUrl = URL(string: UIApplication.openSettingsURLString) {
                UIApplication.shared.open(settingsUrl)
              }

              finishOnboarding()

              // TODO: Show URL Bar Onboarding
            },
            label: {
              (Text("Go to System Settings ") + Text(Image(systemName: "arrow.right")))
                .font(.body.weight(.semibold))
                .foregroundColor(Color(.white))
                .padding()
                .foregroundStyle(.white)
                .frame(maxWidth: .infinity)
                .background(Color(braveSystemName: .buttonBackground))
            }
          )
          .clipShape(RoundedRectangle(cornerRadius: 12.0))
          .overlay(RoundedRectangle(cornerRadius: 12.0).strokeBorder(Color.black.opacity(0.2)))

          Button(action: {
            finishOnboarding()

            // TODO: Show URL Bar Onboarding
          }) {
            Text("I’ll do this Later...")
              .font(.subheadline.weight(.semibold))
              .foregroundColor(Color(braveSystemName: .textSecondary))
          }
          .background(Color.clear)
          .padding(.bottom, 8)

          FocusStepsPagingIndicator(totalPages: 4, activeIndex: .constant(3))
        }
        .padding(.bottom, 20)
      }
      .padding(.horizontal, 20)
      .background(Color(braveSystemName: .pageBackground))
    }
    .overlay(alignment: .topTrailing) {
      Button(
        action: {
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
    .onChange(of: shouldDismiss) { shouldDismiss in
      if shouldDismiss {
        Preferences.Onboarding.basicOnboardingCompleted.value = OnboardingState.completed.rawValue
        Preferences.AppState.shouldDeferPromotedPurchase.value = false
        
        finishOnboarding()
      }
    }
    .navigationBarHidden(true)
  }
  
  private func finishOnboarding() {
    Preferences.Onboarding.basicOnboardingCompleted.value = OnboardingState.completed.rawValue
    Preferences.AppState.shouldDeferPromotedPurchase.value = false
    
    dismiss()
  }
}

struct FocusSystemSettingsView_Previews: PreviewProvider {
  static var previews: some View {
    FocusSystemSettingsView()
  }
}
