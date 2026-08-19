// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Lottie
import Preferences
import SafariServices
import Strings
import SwiftUI

struct MetricsOptInGraphicsView: View {
  @Bindable var state: MetricsOptInOnboardingStep.State

  @Environment(\.onboardingEnvironment.localState) private var localState
  @Environment(\.onboardingEnvironment.p3aUtils) private var p3aUtils

  private var isMetricsReportingManaged: Bool {
    localState?.isManagedPreference(forPath: kMetricsReportingEnabled) == true
  }

  private var isP3AManaged: Bool {
    p3aUtils?.isP3APreferenceManaged == true
  }

  var body: some View {
    VStack(spacing: 0) {
      Spacer()
      Image("focus-product-insight", bundle: .module)
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(maxHeight: 250)
      Spacer()
      VStack(spacing: 16) {
        if !isMetricsReportingManaged {
          Toggle(isOn: $state.isCrashReportingEnabled) {
            Text(Strings.FocusOnboarding.sendCrashReportsToggleTitle)
              .font(.headline)
            Text(Strings.FocusOnboarding.sendCrashReportsToggleDescription)
              .font(.footnote)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
        }
        if !isP3AManaged {
          Toggle(isOn: $state.isP3AEnabled) {
            Text(Strings.FocusOnboarding.p3aToggleTitle)
              .font(.headline)
            Text(Strings.FocusOnboarding.p3aToggleDescription)
              .font(.footnote)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
        }
      }
      .padding(24)
      .fixedSize(horizontal: false, vertical: true)
      .foregroundStyle(Color(braveSystemName: .textPrimary))
      .tint(Color(braveSystemName: .primitivePrimary35))
    }
  }
}

struct MetricsOptInActionsView: View {
  var state: MetricsOptInOnboardingStep.State
  var continueHandler: () -> Void

  @Environment(\.onboardingEnvironment.p3aUtils) private var p3aUtils
  @Environment(\.onboardingEnvironment.localState) private var localState
  @Environment(\.onboardingEnvironment.attributionManager) private var attributionManager

  var body: some View {
    Button {
      if let p3aUtils = p3aUtils {
        p3aUtils.isP3AEnabled = state.isP3AEnabled
        attributionManager?.pingDAUServer(p3aUtils.isP3AEnabled)
        p3aUtils.isNoticeAcknowledged = true
      }
      localState?.set(state.isCrashReportingEnabled, forPath: kMetricsReportingEnabled)
      Preferences.Onboarding.p3aOnboardingShown.value = true
      continueHandler()
    } label: {
      Text(Strings.FocusOnboarding.startBrowseActionButtonTitle)
        .frame(maxWidth: .infinity)
    }
    .primaryContinueAction()
  }
}

public struct MetricsOptInOnboardingStep: OnboardingStep {
  @Observable class State {
    var isCrashReportingEnabled: Bool = true
    var isP3AEnabled: Bool = true
  }
  public var id: String = "metrics-opt-in"
  private var state: State = .init()

  public func makeTitle() -> some View {
    OnboardingTitleView(
      title: Strings.FocusOnboarding.p3aScreenTitle,
      subtitle: Strings.FocusOnboarding.p3aScreenDescription
    )
  }
  public func makeGraphic() -> some View {
    MetricsOptInGraphicsView(state: state)
  }
  public func makeActions(continueHandler: @escaping () -> Void) -> some View {
    MetricsOptInActionsView(state: state, continueHandler: continueHandler)
  }
}

extension OnboardingStep where Self == MetricsOptInOnboardingStep {
  public static var metricsOptIn: Self { .init() }
}

#if DEBUG
#Preview {
  OnboardingStepView(step: .metricsOptIn)
}
#endif
