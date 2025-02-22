// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Lottie
import Preferences
import SafariServices
import Strings
import SwiftUI

struct P3AOptInGraphicsView: View {
  @Environment(\.onboardingEnvironment.p3aUtils) private var p3aUtils
  @State private var isDisplayingP3AHelp = false
  @State private var isP3AEnabled: Bool = false

  var body: some View {
    VStack(spacing: 0) {
      Spacer()
      Image("focus-product-insight", bundle: .module)
        .resizable()
        .aspectRatio(contentMode: .fit)
      Spacer()
      Toggle(LocalizedStringKey(Strings.FocusOnboarding.p3aToggleTitle), isOn: $isP3AEnabled)
        .padding(24)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .tint(Color(braveSystemName: .primitivePrimary35))
        .fixedSize(horizontal: false, vertical: true)
      Divider()
      Text(LocalizedStringKey(Strings.FocusOnboarding.p3aToggleDescription))
        .font(.footnote)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
        .padding()
        .multilineTextAlignment(.center)
        .tint(Color(braveSystemName: .textInteractive))
        .environment(
          \.openURL,
          OpenURLAction { _ in
            isDisplayingP3AHelp = true
            return .handled
          }
        )
    }
    .onAppear {
      isP3AEnabled = p3aUtils?.isP3AEnabled ?? false
    }
    .onChange(of: isP3AEnabled) { newValue in
      p3aUtils?.isP3AEnabled = newValue
    }
    .sheet(isPresented: $isDisplayingP3AHelp) {
      FocusSafariControllerView(url: FocusOnboardingConstants.p3aHelpArticle)
    }
  }
}

struct P3AOptInActionsView: View {
  var continueHandler: () -> Void

  @Environment(\.onboardingEnvironment.p3aUtils) private var p3aUtils
  @Environment(\.onboardingEnvironment.attributionManager) private var attributionManager

  var body: some View {
    Button {
      if let p3aUtils = p3aUtils {
        attributionManager?.pingDAUServer(p3aUtils.isP3AEnabled)
        p3aUtils.isNoticeAcknowledged = true
      }
      Preferences.Onboarding.p3aOnboardingShown.value = true
      continueHandler()
    } label: {
      Text(Strings.FocusOnboarding.startBrowseActionButtonTitle)
        .frame(maxWidth: .infinity)
    }
    .primaryContinueAction()
  }
}

private struct FocusSafariControllerView: UIViewControllerRepresentable {
  let url: URL

  func makeUIViewController(context: Context) -> SFSafariViewController {
    return SFSafariViewController(url: url)
  }

  func updateUIViewController(_ uiViewController: SFSafariViewController, context: Context) {}
}

public struct P3AOptInOnboardingStep: OnboardingStep {
  public var id: String = "p3a-opt-in"
  public func makeTitle() -> some View {
    OnboardingTitleView(
      title: Strings.FocusOnboarding.p3aScreenTitle,
      subtitle: Strings.FocusOnboarding.p3aScreenDescription
    )
  }
  public func makeGraphic() -> some View {
    P3AOptInGraphicsView()
  }
  public func makeActions(continueHandler: @escaping () -> Void) -> some View {
    P3AOptInActionsView(continueHandler: continueHandler)
  }
}

extension OnboardingStep where Self == P3AOptInOnboardingStep {
  public static var p3aOptIn: Self { .init() }
}

#if DEBUG
#Preview {
  OnboardingStepView(step: .p3aOptIn)
}
#endif
