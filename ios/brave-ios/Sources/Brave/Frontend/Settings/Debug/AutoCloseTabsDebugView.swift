// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import SwiftUI

struct AutoCloseTabsDebugView: View {
  @ObservedObject private var minutesOverride = Preferences.Debug.autocloseTabsMinutesOverride

  var body: some View {
    List {
      Section {
        HStack {
          Text("Minutes")
          Spacer()
          TextField("Off", value: $minutesOverride.value, format: .number)
            .keyboardType(.numberPad)
            .multilineTextAlignment(.trailing)
        }
      } footer: {
        Text(
          "Overrides the interval of the selected Auto Close Tabs setting. Leave empty to use the selected interval. Auto Close Tabs must not be set to Manually."
        )
      }
    }
    .navigationTitle("Auto Close Tabs")
  }
}
