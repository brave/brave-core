// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Foundation
import Growth
import Onboarding
import Preferences
import SwiftUI

struct RetentionPreferencesDebugMenuView: View {
  private let p3aUtilities: BraveP3AUtils
  private let attributionManager: AttributionManager

  @State private var isOnboardingPresented: Bool = false

  @ObservedObject private var isNewRetentionUser = Preferences.Onboarding.isNewRetentionUser
  @ObservedObject private var ntpCalloutCompleted = Preferences.FullScreenCallout
    .ntpCalloutCompleted
  @ObservedObject private var rewardsCalloutCompleted = Preferences.FullScreenCallout
    .rewardsCalloutCompleted
  @ObservedObject private var defaultBrowserIntroCompleted = Preferences.DefaultBrowserIntro
    .completed

  init(
    p3aUtilities: BraveP3AUtils,
    attributionManager: AttributionManager
  ) {
    self.p3aUtilities = p3aUtilities
    self.attributionManager = attributionManager
  }

  var body: some View {
    Form {
      Section {
        Button("Start Onboarding") {
          isOnboardingPresented = true
        }
        .fullScreenCover(isPresented: $isOnboardingPresented) {
          OnboardingRepresentable(
            p3aUtilities: p3aUtilities,
            attributionManager: attributionManager
          )
        }
      }
      Section {
        Toggle(isOn: $isNewRetentionUser.value.coalesced) {
          VStack(alignment: .leading) {
            Text("Retention User")
            Text(
              "Flag showing if the user installed the application after new onboarding is added."
            )
            .foregroundStyle(Color(braveSystemName: .textSecondary))
            .font(.footnote)
          }
        }
        Toggle(isOn: $ntpCalloutCompleted.value) {
          VStack(alignment: .leading) {
            Text("NTP Education Shown")
            Text("Flag tracking NTP Education should be loaded after onboarding of user.")
              .foregroundStyle(Color(braveSystemName: .textSecondary))
              .font(.footnote)
          }
        }
        Toggle(isOn: $rewardsCalloutCompleted.value) {
          VStack(alignment: .leading) {
            Text("Rewards Callout Shown")
            Text("Flag determining if Rewards callout is shown to user.")
              .foregroundStyle(Color(braveSystemName: .textSecondary))
              .font(.footnote)
          }
        }
        Toggle(isOn: $defaultBrowserIntroCompleted.value) {
          VStack(alignment: .leading) {
            Text("Default Browser Callout Shown")
            Text("Flag determining if DefaultBrowser callout is shown to user.")
              .foregroundStyle(Color(braveSystemName: .textSecondary))
              .font(.footnote)
          }
        }
      } header: {
        Text("Onboarding Debug Menu")
      } footer: {
        Text(
          "These are the preferences that stored in preferences for determining the If certain elements are shown to user."
        )
      }
    }
  }
}

extension Bool? {
  fileprivate var coalesced: Bool {
    get { self ?? false }
    set { self = newValue }
  }
}

private struct OnboardingRepresentable: UIViewControllerRepresentable {
  var p3aUtilities: BraveP3AUtils
  var attributionManager: AttributionManager

  func makeUIViewController(context: Context) -> OnboardingController {
    let env = OnboardingEnvironment(
      p3aUtils: p3aUtilities,
      attributionManager: attributionManager
    )
    var steps: [any OnboardingStep] = [.defaultBrowsing, .blockInterruptions]
    if !p3aUtilities.isP3APreferenceManaged {
      steps.append(.p3aOptIn)
    }
    return OnboardingController(environment: env, steps: steps)
  }
  func updateUIViewController(_ uiViewController: OnboardingController, context: Context) {
  }
}

class RetentionPreferencesDebugMenuViewController: UIHostingController<
  RetentionPreferencesDebugMenuView
>
{
  init(p3aUtilities: BraveP3AUtils, attributionManager: AttributionManager) {
    super.init(
      rootView: RetentionPreferencesDebugMenuView(
        p3aUtilities: p3aUtilities,
        attributionManager: attributionManager
      )
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = "Onboarding Debug Menu"
  }
}
