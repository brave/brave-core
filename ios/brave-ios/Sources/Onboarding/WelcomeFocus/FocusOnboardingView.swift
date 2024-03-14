// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import Introspect
import Strings
import SwiftUI

public struct FocusOnboardingView: View {
  @State private var splash = true
  @Namespace var namespace

  public init() {}

  public var body: some View {
    VStack {
      if splash {
        FocusSplashScreenView(namespace: namespace)
      } else {
        FocusStepsView(namespace: namespace)
      }
    }
    .onAppear {
      withAnimation(.easeInOut(duration: 1.5).delay(1.5)) {
        splash = false
      }
    }
  }
}

struct FocusOnboardingView_Previews: PreviewProvider {
  static var previews: some View {
    FocusOnboardingView()
  }
}
