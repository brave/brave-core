// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import LocalAuthentication
import SwiftUI

struct OnboardingCompletedView: View {
  var keyringStore: KeyringStore

  @State private var isBiometricCompleted: Bool = false

  private var isBiometricsAvailable: Bool {
    LAContext().canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil)
  }

  var body: some View {
    if isBiometricsAvailable && !isBiometricCompleted {
      BiometricView(
        keyringStore: keyringStore
      ) { isBiometricCompleted = true }
    } else {
      GeometryReader { geometry in
        ScrollView {
          VStack {
            Image("wallet-onboarding-complete", bundle: .module)
            Text(Strings.Wallet.onboardingCompletedTitle)
              .font(.title)
              .foregroundColor(.primary)
              .fixedSize(horizontal: false, vertical: true)
            Text(Strings.Wallet.onboardingCompletedSubTitle)
              .font(.subheadline)
              .foregroundColor(.secondary)
              .fixedSize(horizontal: false, vertical: true)
              .padding(.top, 4)
            Button {
              keyringStore.markOnboardingCompleted()
            } label: {
              Text(Strings.Wallet.onboardingCompletedButtonTitle)
                .frame(maxWidth: .infinity)
            }
            .buttonStyle(BraveFilledButtonStyle(size: .large))
            .padding(.top, 84)
          }
          .frame(maxWidth: .infinity, minHeight: geometry.size.height)
          .padding(.horizontal, 20)
        }
        .onTapGesture {
          keyringStore.markOnboardingCompleted()
        }
        .background(
          Image("wallet-background", bundle: .module)
            .resizable()
            .aspectRatio(contentMode: .fill)
            .edgesIgnoringSafeArea(.all)
        )
        .interactiveDismissDisabled()
        .transparentNavigationBar()
      }
    }
  }
}

#if DEBUG
struct OnboardingCompletedView_Previews: PreviewProvider {
  static var previews: some View {
    OnboardingCompletedView(keyringStore: .previewStore)
  }
}
#endif
