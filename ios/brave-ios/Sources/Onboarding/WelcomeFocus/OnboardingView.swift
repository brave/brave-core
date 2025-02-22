// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Strings
import SwiftUI

/// How the onboarding screen is currently being displayed
enum OnboardingLayoutStyle {
  /// Onboarding content is being displayed fullscreen
  ///
  /// This style is only used for compact horizontal size classes where the height does not exceed
  /// a given threshold
  case fullscreen
  /// Onboarding content is being displayed insetted in the center of the screen
  ///
  /// This style is shown when the horizontal size class is regular and not in landscape
  case inset
  /// Onboarding content is being displayed insetted in the center of the screen with text and
  /// graphics being displayed side-by-side.
  ///
  /// This style is shown when the horizontal size class is regular and the device is landscape
  case columnInset

  /// Whether or not onboarding content is being displayed inset
  var isInset: Bool {
    return self != .fullscreen
  }
}

extension EnvironmentValues {
  /// The current layout style being used for onboarding
  @Entry var onboardingLayoutStyle: OnboardingLayoutStyle = .fullscreen
  /// A namespace that can be used for matched geometry effects between onboarding steps
  @Entry var onboardingNamespace: Namespace.ID?
  /// An environment containing dependencies that may be needed for indiviudal steps
  @Entry var onboardingEnvironment: OnboardingEnvironment = .init()
}

/// A view controller that embeds an onboarding SwiftUI view hierarchy
public class OnboardingController: UIHostingController<OnboardingRootView> {
  public init(
    environment: OnboardingEnvironment,
    steps: [any OnboardingStep] = .allSteps,
    showSplashScreen: Bool = true,
    showDismissButton: Bool = false,
    onCompletion: @escaping () -> Void = {}
  ) {
    super.init(
      rootView: .init(
        environment: environment,
        steps: steps,
        showSplashScreen: showSplashScreen,
        showDismissButton: showDismissButton,
        onCompletion: onCompletion
      )
    )
    modalPresentationStyle = .fullScreen
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  public override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    return .portrait
  }

  public override var preferredInterfaceOrientationForPresentation: UIInterfaceOrientation {
    return .portrait
  }

  public override var shouldAutorotate: Bool {
    return false
  }
}

/// A root SwiftUI heirarchy that allows the embedded view hierarchies to use the effective geometry
/// environment values (which we need orientation for) and injects onboarding dependencies
public struct OnboardingRootView: View {
  public var environment: OnboardingEnvironment
  public var steps: [any OnboardingStep]
  public var showSplashScreen: Bool
  public var showDismissButton: Bool
  public var onCompletion: () -> Void

  public init(
    environment: OnboardingEnvironment,
    steps: [any OnboardingStep],
    showSplashScreen: Bool,
    showDismissButton: Bool,
    onCompletion: @escaping () -> Void = {}
  ) {
    assert(!steps.isEmpty, "You must pass in at least 1 step")
    self.environment = environment
    self.steps = steps
    self.showSplashScreen = showSplashScreen
    self.showDismissButton = showDismissButton
    self.onCompletion = onCompletion
  }

  /// Computes the layout style given the effective geometry, size classes, and overal available
  /// space and injects the layout into the environment
  struct OnboardingLayoutContainerView: View {
    var steps: [any OnboardingStep]
    var showSplashScreen: Bool
    var showDismissButton: Bool
    var onCompletion: () -> Void

    @Environment(\.interfaceOrientation) private var interfaceOrientation
    @Environment(\.horizontalSizeClass) private var horizontalSizeClass
    @Namespace private var namespace

    private func layoutStyle(for size: CGSize) -> OnboardingLayoutStyle {
      if horizontalSizeClass == .regular {
        if interfaceOrientation.isLandscape {
          return .columnInset
        }
        return .inset
      }
      if size.height > 1000 {
        return .inset
      }
      return .fullscreen
    }

    var body: some View {
      GeometryReader { proxy in
        let layoutStyle = layoutStyle(for: proxy.size)
        OnboardingView(
          steps: steps,
          showSplashScreen: showSplashScreen,
          showDismissButton: showDismissButton,
          onCompletion: onCompletion
        )
        .dynamicTypeSize(
          ...(layoutStyle == .fullscreen
            ? DynamicTypeSize.accessibility1 : DynamicTypeSize.accessibility2)
        )
        .environment(\.onboardingLayoutStyle, layoutStyle)
        .environment(\.onboardingNamespace, namespace)
      }
    }
  }

  public var body: some View {
    OnboardingLayoutContainerView(
      steps: steps,
      showSplashScreen: showSplashScreen,
      showDismissButton: showDismissButton,
      onCompletion: onCompletion
    )
    .prepareEffectiveGeometryEnvironment()
    .environment(\.onboardingEnvironment, environment)
  }
}

/// An onboarding view container that displays onboarding steps for a specific layout style
///
/// This View handles moving between steps and where to show a step's title/graphic/action views
struct OnboardingView: View {
  var steps: [any OnboardingStep]
  var showSplashScreen: Bool
  var showDismissButton: Bool
  var onCompletion: () -> Void

  /// Animation phases between the splash screen and steps
  enum SplashPhase {
    /// Splash is up, steps are hidden
    case visible
    /// Splash is scaling up on the way of animating out
    case scalingUp
    /// Splash is now fully hidden
    case animatingOut
  }

  /// The current splash animation phase or nil if the splash screen is not visible at all
  @State private var splashPhase: SplashPhase?
  @State private var activeStepIndex: Int = 0

  @Environment(\.onboardingLayoutStyle) private var onboardingLayoutStyle
  @Environment(\.dismiss) private var dismiss

  private var isSplashVisible: Bool {
    splashPhase != nil
  }

  var body: some View {
    ZStack {
      OnboardingStepView(
        step: steps[activeStepIndex],
        isSplashVisible: isSplashVisible,
        onContinue: {
          let nextStep = activeStepIndex + 1
          if nextStep == steps.endIndex {
            onCompletion()
            dismiss()
            return
          }
          withAnimation(.bouncy) {
            activeStepIndex = nextStep
          }
        }
      )
      .overlay(alignment: .topTrailing) {
        if showDismissButton {
          Button {
            onCompletion()
            dismiss()
          } label: {
            Label(Strings.close, braveSystemImage: "leo.close.circle-filled")
              .foregroundStyle(Color(braveSystemName: .neutral5))
              .imageScale(.large)
              .overlay {
                Image(braveSystemName: "leo.close")
                  .foregroundStyle(Color(braveSystemName: .iconSecondary))
                  .imageScale(.small)
              }
          }
          .font(.title3)
          .labelStyle(.iconOnly)
          .opacity(isSplashVisible ? 0 : 1)
          .padding()
        }
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .opacity(!onboardingLayoutStyle.isInset || splashPhase == nil ? 1 : 0)
      .padding(onboardingLayoutStyle.isInset ? 16 : 0)
      if isSplashVisible {
        OnboardingSplashView()
          .scaleEffect(onboardingLayoutStyle.isInset && splashPhase == .scalingUp ? 1.1 : 1)
          .transition(
            onboardingLayoutStyle.isInset ? .scale(scale: 0.5).combined(with: .opacity) : .offset()
          )
          .zIndex(2)
          .task {
            try? await Task.sleep(for: .seconds(1.25))
            if onboardingLayoutStyle.isInset {
              withAnimation(.snappy) {
                splashPhase = .scalingUp
              }
              try? await Task.sleep(for: .seconds(0.15))
            }
            withAnimation {
              splashPhase = .animatingOut
            }
            if onboardingLayoutStyle.isInset {
              try? await Task.sleep(for: .seconds(0.5))
            }
            withAnimation {
              splashPhase = nil
            }
          }
      }
    }
    .background {
      if onboardingLayoutStyle.isInset {
        if isSplashVisible {
          Color(braveSystemName: .iosBrowserChromeBackgroundIos)
            .ignoresSafeArea()
        } else {
          Image("focus-background-large", bundle: .module)
            .resizable()
            .ignoresSafeArea()
        }
      } else {
        Color(braveSystemName: .iosBrowserChromeBackgroundIos)
          .ignoresSafeArea()
      }
    }
    .onAppear {
      splashPhase = showSplashScreen ? .visible : nil
    }
  }
}

#if DEBUG
#Preview {
  OnboardingRootView(
    environment: .init(),
    steps: .allSteps,
    showSplashScreen: true,
    showDismissButton: true
  )
}
#endif
