// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Lottie
import SwiftUI

struct DefaultBrowserGraphicView: View {
  @Environment(\.colorScheme) private var colorScheme

  var body: some View {
    LottieView(
      animation: .named(
        colorScheme == .dark ? "browser-default-dark" : "browser-default-light",
        bundle: .module,
        subdirectory: "LottieAssets"
      )
    )
    .resizable()
    .playing(loopMode: .loop)
    .aspectRatio(contentMode: .fit)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
  }
}

struct DefaultBrowserActions: View {
  var continueHandler: () -> Void
  var body: some View {
    HStack {
      Button {
        continueHandler()
      } label: {
        Text(Strings.FocusOnboarding.notNowActionButtonTitle)
          .frame(maxWidth: .infinity, maxHeight: .infinity)
      }
      .buttonStyle(BraveOutlineButtonStyle(size: .large))
      Button {
        if let openSettingsURL = URL(string: UIApplication.openSettingsURLString) {
          UIApplication.shared.open(openSettingsURL)
        }
        continueHandler()
      } label: {
        Text(Strings.FocusOnboarding.systemSettingsButtonTitle)
          .frame(maxWidth: .infinity, maxHeight: .infinity)
      }
      .primaryContinueAction()
    }
    .fixedSize(horizontal: false, vertical: true)
  }
}

public struct DefaultBrowserOnboardingStep: OnboardingStep {
  public var id: String = "default-browsing"
  public func makeTitle() -> some View {
    OnboardingTitleView(
      title: Strings.FocusOnboarding.defaultBrowserScreenTitle,
      subtitle: Strings.FocusOnboarding.defaultBrowserScreenDescription
    )
  }
  public func makeGraphic() -> some View {
    DefaultBrowserGraphicView()
  }
  public func makeActions(continueHandler: @escaping () -> Void) -> some View {
    DefaultBrowserActions(continueHandler: continueHandler)
  }
}

extension OnboardingStep where Self == DefaultBrowserOnboardingStep {
  public static var defaultBrowsing: Self { .init() }
}

#if DEBUG
#Preview {
  OnboardingStepView(step: .defaultBrowsing)
}
#endif
