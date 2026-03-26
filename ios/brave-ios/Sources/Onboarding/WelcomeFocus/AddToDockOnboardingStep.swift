// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Lottie
import Strings
import SwiftUI

struct AddToDockGraphicView: View {
  @Environment(\.colorScheme) private var colorScheme

  var body: some View {
    GeometryReader { proxy in
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
      .frame(width: proxy.size.width * 0.90, height: proxy.size.height * 0.90)
      .position(x: proxy.size.width * 0.50, y: proxy.size.height * 0.50)
    }
  }
}

public struct AddToDockOnboardingStep: OnboardingStep {
  public var id: String = "add-to-dock"
  public func makeTitle() -> some View {
    OnboardingTitleView(
      title: Strings.FocusOnboarding.addToDockScreenTitle,
      subtitle: Strings.FocusOnboarding.addToDockScreenDescription
    )
  }
  public func makeGraphic() -> some View {
    AddToDockGraphicView()
  }
  public func makeActions(continueHandler: @escaping () -> Void) -> some View {
    Button {
      continueHandler()
    } label: {
      Text(Strings.FocusOnboarding.continueButtonTitle)
        .frame(maxWidth: .infinity)
    }
    .primaryContinueAction()
  }
}

extension OnboardingStep where Self == AddToDockOnboardingStep {
  public static var addToDock: Self { .init() }
}

#if DEBUG
#Preview {
  OnboardingStepView(step: .addToDock)
}
#endif
