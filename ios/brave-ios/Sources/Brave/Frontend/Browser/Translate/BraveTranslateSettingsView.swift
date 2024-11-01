// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
import Preferences
import Shared
import SwiftUI

struct BraveTranslateSettingsView: View {
  @ObservedObject
  private var translateEnabled = Preferences.Translate.translateEnabled

  var body: some View {
    Form {
      Section {
        Toggle(isOn: $translateEnabled.value) {
          Text(Strings.BraveTranslate.settingsTranslateEnabledOptionTitle)
        }
        .toggleStyle(SwitchToggleStyle(tint: .accentColor))
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.BraveTranslate.settingsTranslateEnabledOptionDescription)
      }
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle(Strings.BraveTranslate.settingsScreenTitle)
  }
}

#if DEBUG
struct BraveTranslateSettingsView_Previews: PreviewProvider {
  static var previews: some View {
    BraveTranslateSettingsView()
  }
}
#endif
