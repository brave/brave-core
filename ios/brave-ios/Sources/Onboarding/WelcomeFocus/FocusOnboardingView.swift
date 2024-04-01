// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import Foundation
import Growth
import Introspect
import Strings
import SwiftUI

public struct FocusOnboardingView: View {
  @State private var isSplashViewPresented = true
  @Namespace var namespace

  private let attributionManager: AttributionManager?
  private let p3aUtilities: BraveP3AUtils?

  public init(attributionManager: AttributionManager? = nil, p3aUtilities: BraveP3AUtils? = nil) {
    self.attributionManager = attributionManager
    self.p3aUtilities = p3aUtilities
  }

  public var body: some View {
    VStack {
      if isSplashViewPresented {
        FocusSplashScreenView(namespace: namespace)
      } else {
        FocusStepsView(
          namespace: namespace,
          attributionManager: attributionManager,
          p3aUtilities: p3aUtilities
        )
      }
    }
    .onAppear {
      withAnimation(.easeInOut(duration: 0.75).delay(1.5)) {
        isSplashViewPresented = false
      }
    }
  }
}

struct FocusOnboardingView_Previews: PreviewProvider {
  static var previews: some View {
    FocusOnboardingView()
  }
}
