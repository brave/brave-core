// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// A single step in the onboarding
///
/// Each onboarding step shows 3 separate components: A title, a graphic/illustration, and actions.
/// Since the design has these parts shift around based on layout and orientation these views have
/// to be separate
///
/// To make a new step, create a type that conforms to `OnboardingStep` and make the appropriate
/// views required. Typically a `Title` will be a `OnboardingTitleView` that matches the standard
/// style, `Graphic` would be an animation or image that fills the space and `Action` will be one
/// or more `Button`
public protocol OnboardingStep: Identifiable {
  associatedtype Title: View
  associatedtype Graphic: View
  associatedtype Actions: View

  /// The id assoicated with this step
  var id: String { get }
  /// Create a View that appears either above the graphic or on the top leading position
  @ViewBuilder func makeTitle() -> Title
  /// Create a View which displays some graphic/illustration (or any additional components) that
  /// display below the title or in the trailing column on iPad in landscape
  @ViewBuilder func makeGraphic() -> Graphic
  /// Create a View which displays actions that the step can take. Call `continueHandler` when an
  /// action is taken that would shift onboarding to the next step.
  ///
  /// Use the `View.primaryContinueAction` modifier to the primary Button that would trigger moving
  /// to the next page to animate nicely between pages
  @ViewBuilder func makeActions(continueHandler: @escaping () -> Void) -> Actions
}

extension [any OnboardingStep] {
  /// All of the standard browser steps
  public static var allSteps: [any OnboardingStep] {
    [.defaultBrowsing, .blockInterruptions, .p3aOptIn]
  }
}

/// A view modifier to animate the primary "Continue" button between pages
private struct PrimaryContinueActionModifier: ViewModifier {
  @Environment(\.onboardingNamespace) private var onboardingNamespace
  @Namespace private var fallbackNamespace
  func body(content: Content) -> some View {
    content
      .transition(.offset().combined(with: .opacity))
      .matchedGeometryEffect(id: "continue", in: onboardingNamespace ?? fallbackNamespace)
  }
}

extension View {
  /// Marks a button in an OnboardingStep as the primary continue action to allow it to animate
  /// nicely between steps
  public func primaryContinueAction() -> some View {
    modifier(PrimaryContinueActionModifier())
  }
}

/// The typical onboarding title view which has a title and subtitle with appropriate styling
public struct OnboardingTitleView: View {
  var title: String
  var subtitle: String

  @Environment(\.onboardingLayoutStyle) private var layoutStyle

  @ScaledMetric private var titleFontSize = 24

  public var body: some View {
    VStack(
      alignment: layoutStyle == .columnInset ? .leading : .center,
      spacing: layoutStyle == .columnInset ? 16 : 8
    ) {
      Text(title)
        .font(.system(size: titleFontSize, weight: .semibold))
        .foregroundStyle(Color(braveSystemName: .textPrimary))
      Text(subtitle)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
    }
    .multilineTextAlignment(layoutStyle == .columnInset ? .leading : .center)
  }
}
