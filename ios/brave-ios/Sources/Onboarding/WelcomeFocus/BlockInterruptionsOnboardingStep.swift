// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Lottie
import Strings
import SwiftUI

struct BlockInterruptionsGraphicView: View {
  @Environment(\.colorScheme) private var colorScheme

  var body: some View {
    LottieView(
      animation: .named(
        colorScheme == .dark ? "novideo-ads-dark" : "novideo-ads-light",
        bundle: .module,
        subdirectory: "LottieAssets/\(Locale.current.region?.identifier == "JP" ? "ja" : "en")"
      )
    )
    .resizable()
    .playing(loopMode: .loop)
    .aspectRatio(contentMode: .fit)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
  }
}

public struct BlockInterruptionsOnboardingStep: OnboardingStep {
  public var id: String = "block-interruptions"
  public func makeTitle() -> some View {
    OnboardingTitleView(
      title: Strings.FocusOnboarding.noVideoAdsScreenTitle,
      subtitle: Strings.FocusOnboarding.noVideoAdsScreenDescription
    )
  }
  public func makeGraphic() -> some View {
    BlockInterruptionsGraphicView()
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

extension OnboardingStep where Self == BlockInterruptionsOnboardingStep {
  public static var blockInterruptions: Self { .init() }
}

#if DEBUG
#Preview {
  OnboardingStepView(step: .blockInterruptions)
}
#endif
