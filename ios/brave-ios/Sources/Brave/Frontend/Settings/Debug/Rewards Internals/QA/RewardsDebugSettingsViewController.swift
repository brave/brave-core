// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import Shared
import SwiftUI

private typealias EnvironmentOverride = Preferences.Rewards.EnvironmentOverride

struct RewardsDebugSettingsView: View {
  var rewards: BraveRewards

  @State private var isResetRewardsAlertVisible: Bool = false
  @ObservedObject private var environmentOverride = Preferences.Rewards.environmentOverride

  var body: some View {
    Form {
      Section {
        let isDefaultEnvironmentProd = AppConstants.isOfficialBuild
        LabeledContent("Default", value: isDefaultEnvironmentProd ? "Prod" : "Staging")
        LabeledContent("Override") {
          Picker("", selection: $environmentOverride.value.transformed) {
            ForEach(EnvironmentOverride.sortedCases, id: \.rawValue) { override in
              Text(override.name)
                .tag(override)
            }
          }
          .pickerStyle(.segmented)
        }
      } header: {
        Text("Environment")
      } footer: {
        Text(
          "Changing the environment automatically resets Brave Rewards.\n\nThe app must be force-quit after rewards is reset"
        )
      }
      if let rewardsAPI = rewards.rewardsAPI {
        Section {
          NavigationLink("Internals") {
            RewardsInternalsView(rewardsAPI: rewardsAPI)
          }
        } header: {
          Text("Wallet")
        }
      }
      Section {
        Button("Reset Rewards") {
          Task {
            await rewards.reset()
            isResetRewardsAlertVisible = true
          }
        }
      }
    }
    .alert("Rewards Reset", isPresented: $isResetRewardsAlertVisible) {
      Button("Exit Now", role: .destructive) {
        fatalError()
      }
      Button(Strings.OKString, role: .cancel) {}
    } message: {
      Text("Brave must be restarted to ensure expected Rewards behavior")
    }
    .navigationTitle("Rewards QA Settings")
    .navigationBarTitleDisplayMode(.inline)
    .onChange(of: environmentOverride.value, initial: false) {
      Task {
        await self.rewards.reset()
        isResetRewardsAlertVisible = true
      }
    }
  }
}

extension Int {
  fileprivate var transformed: EnvironmentOverride {
    get { .init(rawValue: self) ?? .none }
    set { self = newValue.rawValue }
  }
}

class RewardsDebugSettingsViewController: UIHostingController<RewardsDebugSettingsView> {
  init(rewards: BraveRewards) {
    super.init(rootView: .init(rewards: rewards))
  }
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  override func viewDidLoad() {
    super.viewDidLoad()
    title = "Rewards QA Settings"
  }
}
