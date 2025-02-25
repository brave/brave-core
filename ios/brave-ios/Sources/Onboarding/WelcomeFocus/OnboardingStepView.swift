// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import SwiftUI

/// Displays a single `OnboardingStep` based on the onboarding layout style
///
/// A note on `AnyView` usage within `OnboardingStepView`: Working with existentials and SwiftUI
/// View's unfortunately requires type erasure due to not being able to store the dynamically
/// create each `OnboardingStep` since they are not `View`s and thus can't use a `ViewBuilder`,
/// so we instead have to use `AnyView`. This usage is typically OK here as we don't care about
/// view identity of each step since they can be explicilty defined using `OnboardingStep.id` and
/// they are typically transitioned and animated as a whole so type information isn't needed.
struct OnboardingStepView: View {
  var step: any OnboardingStep
  var isSplashVisible: Bool = false
  var onContinue: () -> Void = {}

  @Environment(\.onboardingLayoutStyle) private var layoutStyle
  @Environment(\.onboardingNamespace) private var namespace
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @Environment(\.dynamicTypeSize) private var dynamicTypeSize

  @Namespace private var fallbackNamespace

  // We have to use a number higher than 1, 1 does not scale correctly
  @ScaledMetric private var layoutSizeScale = 10

  private var layoutIdealSize: CGSize? {
    if dynamicTypeSize.isAccessibilitySize {
      return nil
    }
    let scale = max(10, layoutSizeScale) / 10
    switch layoutStyle {
    case .fullscreen: return nil
    case .inset: return CGSize(width: 580 * scale, height: 800 * scale)
    case .columnInset: return CGSize(width: 840 * scale, height: 580 * scale)
    }
  }

  var body: some View {
    HStack(spacing: 0) {
      VStack(spacing: layoutStyle == .columnInset ? 40 : 20) {
        HStack {
          if layoutStyle.isInset {
            HStack(spacing: 4) {
              Image(sharedName: "brave.logo")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(height: 56)
                // the logo has some padding in the image itself
                .padding(.leading, layoutStyle == .columnInset ? -8 : 0)
              Image(sharedName: "brave.wordmark")
                .renderingMode(.template)
                .foregroundStyle(Color(braveSystemName: .neutral70))
            }
          } else {
            BraveAppIcon(
              size: 48,
              matchedGeometryInfo: .init(
                namespace: namespace ?? fallbackNamespace,
                isSource: !isSplashVisible
              )
            )
          }
        }
        .zIndex(2)
        .frame(maxWidth: .infinity, alignment: layoutStyle == .columnInset ? .leading : .center)
        VStack(spacing: layoutStyle.isInset && horizontalSizeClass == .regular ? 40 : 28) {
          AnyView(step.makeTitle())
            .id(step.id)
            .frame(maxWidth: .infinity, alignment: layoutStyle == .columnInset ? .leading : .center)
            .transition(.push(from: .trailing))
          if layoutStyle == .columnInset {
            Spacer()
          } else {
            AnyView(step.makeGraphic())
              .frame(maxWidth: .infinity, maxHeight: .infinity)
              .clipShape(.rect(cornerRadius: 16, style: .continuous))
              .background(
                Color(braveSystemName: .iosBrowserBackgroundIos)
                  .shadow(.drop(color: Color(braveSystemName: .elevationPrimary), radius: 0, y: 1))
                  .shadow(
                    .drop(color: Color(braveSystemName: .elevationSecondary), radius: 4, y: 1)
                  ),
                in: .rect(cornerRadius: 16, style: .continuous)
              )
              .transition(.push(from: .trailing))
          }
          AnyView(step.makeActions(continueHandler: onContinue))
            .id(step.id)
            .buttonStyle(BraveFilledButtonStyle(size: .large))
        }
        .opacity(isSplashVisible ? 0 : 1)
        .zIndex(1)
      }
      .padding(layoutStyle.isInset && horizontalSizeClass == .regular ? 40 : 20)
      .clipShape(.rect)
      if layoutStyle == .columnInset {
        AnyView(step.makeGraphic())
          .frame(maxHeight: .infinity)
          .background(Color(braveSystemName: .iosBrowserBackgroundIos))
      }
    }
    .frame(maxWidth: layoutIdealSize?.width, maxHeight: layoutIdealSize?.height)
    .background {
      if layoutStyle.isInset {
        Color(braveSystemName: .iosBrowserElevatedIos)
      }
    }
    .clipShape(.rect(cornerRadius: layoutStyle.isInset ? 24 : 0, style: .continuous))
  }
}

#if DEBUG
#Preview {
  OnboardingStepView(step: .defaultBrowsing)
}
#endif
