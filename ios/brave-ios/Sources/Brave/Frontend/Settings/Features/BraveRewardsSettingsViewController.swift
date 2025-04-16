// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Combine
import Shared
import SwiftUI
import UIKit
import Web

private class BraveRewardsSettingsViewModel: ObservableObject {
  let rewards: BraveRewards
  private var rewardsObserver: RewardsObserver?
  private var isEnabledObserver: NSKeyValueObservation?

  init(rewards: BraveRewards) {
    self.rewards = rewards

    if let rewardsAPI = rewards.rewardsAPI {
      isWalletInitialized = rewardsAPI.isInitialized
    }

    isEnabledObserver = rewards.ads.observe(\.isEnabled, options: [.new]) { [weak self] _, change in
      if let newValue = change.newValue {
        self?.isRewardsEnabled = newValue
      }
    }
  }

  var isRewardsEnabled: Bool {
    get {
      rewards.isEnabled || rewards.isTurningOnRewards
    }
    set {
      if rewards.rewardsAPI == nil {
        rewards.startRewardsService { [weak self] in
          guard let self else { return }
          if let rewardsAPI = self.rewards.rewardsAPI {
            let observer = RewardsObserver(rewardsAPI: rewardsAPI)
            rewardsAPI.add(observer)
            self.rewardsObserver = observer
            observer.walletInitalized = { [weak self] _ in
              DispatchQueue.main.async {
                self?.isWalletInitialized = true
              }
            }
          }
          self.rewards.isEnabled = newValue
          self.objectWillChange.send()
        }
      } else {
        rewards.isEnabled = newValue
        self.objectWillChange.send()
      }
    }
  }

  @Published private(set) var isWalletInitialized: Bool = false
}

struct BraveRewardsSettingsView: View {
  @ObservedObject private var model: BraveRewardsSettingsViewModel
  fileprivate init(model: BraveRewardsSettingsViewModel) {
    self.model = model
  }
  var body: some View {
    Form {
      Section {
        Toggle(isOn: $model.isRewardsEnabled) {
          VStack(alignment: .leading) {
            Text(Strings.Rewards.settingsToggleTitle)
            Text(Strings.Rewards.settingsToggleMessage)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
              .font(.footnote)
          }
        }
        .tint(Color(braveSystemName: .primary50))
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      if model.isWalletInitialized, let rewardsAPI = model.rewards.rewardsAPI {
        Section {
          NavigationLink {
            UIKitController(RewardsInternalsViewController(rewardsAPI: rewardsAPI))
          } label: {
            Text(Strings.RewardsInternals.title)
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
    }
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
  }
}

class BraveRewardsSettingsViewController: UIHostingController<BraveRewardsSettingsView> {
  init(rewards: BraveRewards) {
    super.init(rootView: BraveRewardsSettingsView(model: .init(rewards: rewards)))
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = Strings.braveRewardsTitle
  }
}
