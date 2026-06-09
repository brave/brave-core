// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Preferences
import Shared
import SwiftUI

struct PrivacyReportSettingsView: View {

  @ObservedObject private var shieldsDataEnabled = Preferences.PrivacyReports.captureShieldsData
  @ObservedObject private var vpnAlertsEnabled = Preferences.PrivacyReports.captureVPNAlerts

  @State private var showClearDataPrompt: Bool = false

  var body: some View {
    Form {
      Section(footer: Text(Strings.PrivacyHub.settingsEnableShieldsFooter)) {
        Toggle(Strings.PrivacyHub.settingsEnableShieldsTitle, isOn: $shieldsDataEnabled.value)
          .tint(Color(braveSystemName: .primitivePrimary40))
      }
      Section(footer: Text(Strings.PrivacyHub.settingsEnableVPNAlertsFooter)) {
        Toggle(Strings.PrivacyHub.settingsEnableVPNAlertsTitle, isOn: $vpnAlertsEnabled.value)
          .tint(Color(braveSystemName: .primitivePrimary40))
      }
      Section(footer: Text(Strings.PrivacyHub.settingsSlearDataFooter)) {
        HStack {
          Button(
            action: {
              showClearDataPrompt = true
            },
            label: {
              Text(Strings.PrivacyHub.settingsSlearDataTitle)
                .frame(maxWidth: .infinity, alignment: .leading)
                .foregroundColor(Color.red)
            }
          )
          .actionSheet(isPresented: $showClearDataPrompt) {
            // Currently .actionSheet does not allow you leave empty title for the sheet.
            // This could get converted to .confirmationPrompt or Menu with destructive buttons
            // once iOS 15 is minimum supported version
            .init(
              title: Text(Strings.PrivacyHub.clearAllDataPrompt),
              buttons: [
                .destructive(
                  Text(Strings.yes),
                  action: {
                    PrivacyReportsManager.clearAllData()
                  }
                ),
                .cancel(),
              ]
            )
          }

        }
      }
    }
    .navigationTitle(Strings.PrivacyHub.privacyReportsTitle)
    .listStyle(.insetGrouped)
  }
}

#if DEBUG
struct PrivacyReportSettingsView_Previews: PreviewProvider {
  static var previews: some View {
    PrivacyReportSettingsView()
  }
}
#endif
