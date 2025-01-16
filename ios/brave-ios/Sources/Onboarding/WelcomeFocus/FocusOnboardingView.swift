// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import Foundation
import Growth
import Introspect
import Preferences
import Strings
import SwiftUI

public struct FocusOnboardingView: View {
  @Namespace var namespace
  @Environment(\.dismiss) private var dismiss

  @State private var shouldDismiss = false
  @State private var isSystemSettingsComplete = false
  @State private var isSplashViewPresented = true

  private let attributionManager: AttributionManager?
  private let p3aUtilities: BraveP3AUtils?

  public init(attributionManager: AttributionManager? = nil, p3aUtilities: BraveP3AUtils? = nil) {
    self.attributionManager = attributionManager
    self.p3aUtilities = p3aUtilities
  }

  public var body: some View {
    NavigationView {
      VStack {
        if isSplashViewPresented {
          FocusSplashScreenView(namespace: namespace)
        } else {
          FocusSystemSettingsView(
            namespace: namespace,
            screenType: .onboarding,
            isCompleted: $isSystemSettingsComplete,
            shouldDismiss: $shouldDismiss
          )
        }
      }
      .background {
        NavigationLink("", isActive: $isSystemSettingsComplete) {
          FocusStepsView(
            namespace: namespace,
            attributionManager: attributionManager,
            p3aUtilities: p3aUtilities,
            shouldDismiss: $shouldDismiss
          )
          .toolbar(.hidden, for: .navigationBar)
        }
      }
      .onAppear {
        withAnimation(.easeInOut(duration: 0.75).delay(1.0)) {
          isSplashViewPresented = false
        }
      }
      .onChange(of: shouldDismiss) { shouldDismiss in
        if shouldDismiss {
          Preferences.Onboarding.basicOnboardingCompleted.value = OnboardingState.completed.rawValue
          Preferences.AppState.shouldDeferPromotedPurchase.value = false
          Preferences.FocusOnboarding.focusOnboardingFinished.value = true

          dismiss()
        }
      }
      .toolbar(.hidden, for: .navigationBar)
    }
    .navigationViewStyle(StackNavigationViewStyle())
  }
}

#if DEBUG
struct FocusOnboardingView_Previews: PreviewProvider {
  static var previews: some View {
    FocusOnboardingView()
  }
}
#endif
