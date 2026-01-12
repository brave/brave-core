// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import Strings
import SwiftUI

public struct OriginSettingsView: View {
  @Bindable private var viewModel: OriginSettingsViewModel

  public init(service: any BraveOriginService) {
    viewModel = .init(service: service)
  }

  public var body: some View {
    Form {
      Section {
        Toggle(isOn: $viewModel.isRewardsDisabled.inversed) {
          Label(Strings.Origin.rewardsLabel, braveSystemImage: "leo.product.bat-outline")
        }
      } header: {
        Text(Strings.Origin.adsHeader)
      }
      Section {
        Toggle(isOn: $viewModel.isP3AEnabled) {
          Label(Strings.Origin.privacyPreservingAnalyticsLabel, braveSystemImage: "leo.bar.chart")
        }
        Toggle(isOn: $viewModel.isStatsPingEnabled) {
          Label(Strings.Origin.statisticsReportingLabel, braveSystemImage: "leo.bar.chart")
        }
      } header: {
        Text(Strings.Origin.analyticsHeader)
      }
      Section {
        Toggle(isOn: $viewModel.isAIChatEnabled) {
          Label(Strings.Origin.leoAILabel, braveSystemImage: "leo.product.brave-leo")
        }
        Toggle(isOn: $viewModel.isNewsDisabled.inversed) {
          Label(Strings.Origin.newsLabel, braveSystemImage: "leo.product.brave-news")
        }
        Toggle(isOn: $viewModel.isTalkDisabled.inversed) {
          Label(Strings.Origin.talkLabel, braveSystemImage: "leo.product.brave-talk")
        }
        Toggle(isOn: $viewModel.isVPNDisabled.inversed) {
          Label(Strings.Origin.vpnLabel, braveSystemImage: "leo.product.vpn")
        }
        Toggle(isOn: $viewModel.isWalletDisabled.inversed) {
          Label(Strings.Origin.walletLabel, braveSystemImage: "leo.product.brave-wallet")
        }
      } header: {
        Text(Strings.Origin.featuresHeader)
      } footer: {
        VStack(alignment: .leading, spacing: 12) {
          // featuresFooter contains markdown
          Text(LocalizedStringKey(Strings.Origin.featuresFooter))
        }
      }
      Section {
        Button(role: .destructive) {
          // Would reset all of the settings
        } label: {
          Text(Strings.Origin.resetToDefaultsButton)
        }
      }
    }
    .navigationTitle(Strings.Origin.originProductName)
    .navigationBarTitleDisplayMode(.inline)
  }
}

extension Bool {
  // Inverses the boolean value, for use in a Binding<Bool> such as when a toggle enables a feature
  // when toggled on, but the pref/policy is stored as disabled = true
  fileprivate var inversed: Bool {
    get { !self }
    set { self = !newValue }
  }
}

#if DEBUG
private class MockOriginService: BraveOriginService {
  private var values: [BraveOriginPolicyKey: Bool] = [:]
  func isPolicyControlled(byBraveOrigin policyKey: BraveOriginPolicyKey) -> Bool {
    return false
  }
  func setPolicyValue(_ policyKey: BraveOriginPolicyKey, value: Bool) -> Bool {
    values[policyKey] = value
    return true
  }
  func getPolicyValue(_ policyKey: BraveOriginPolicyKey) -> NSNumber? {
    values[policyKey].map(NSNumber.init)
  }
}
#Preview {
  NavigationStack {
    OriginSettingsView(service: MockOriginService())
  }
}
#endif
