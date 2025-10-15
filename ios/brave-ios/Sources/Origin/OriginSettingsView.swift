// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Strings
import SwiftUI

public struct OriginSettingsView: View {
  public init() {
  }

  public var body: some View {
    Form {
      Section {
        Toggle(isOn: .constant(false)) {
          Label(Strings.Origin.rewardsLabel, braveSystemImage: "leo.product.bat-outline")
        }
      } header: {
        Text(Strings.Origin.adsHeader)
      }
      Section {
        Toggle(isOn: .constant(false)) {
          Label(Strings.Origin.privacyPreservingAnalyticsLabel, braveSystemImage: "leo.bar.chart")
        }
        Toggle(isOn: .constant(false)) {
          Label(Strings.Origin.statisticsReportingLabel, braveSystemImage: "leo.bar.chart")
        }
      } header: {
        Text(Strings.Origin.analyticsHeader)
      }
      Section {
        Toggle(isOn: .constant(false)) {
          Label(Strings.Origin.leoAILabel, braveSystemImage: "leo.product.brave-leo")
        }
        Toggle(isOn: .constant(false)) {
          Label(Strings.Origin.newsLabel, braveSystemImage: "leo.product.brave-news")
        }
        Toggle(isOn: .constant(false)) {
          Label(Strings.Origin.playlistLabel, braveSystemImage: "leo.product.playlist")
        }
        Toggle(isOn: .constant(false)) {
          Label(Strings.Origin.talkLabel, braveSystemImage: "leo.product.brave-talk")
        }
        Toggle(isOn: .constant(false)) {
          Label(Strings.Origin.vpnLabel, braveSystemImage: "leo.product.vpn")
        }
        Toggle(isOn: .constant(false)) {
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

#if DEBUG
#Preview {
  NavigationStack {
    OriginSettingsView()
  }
}
#endif
