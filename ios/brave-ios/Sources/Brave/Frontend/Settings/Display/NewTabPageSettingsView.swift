// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import Preferences
import SwiftUI

struct NewTabPageSettingsView: View {
  @ObservedObject private var showNewTabFavourites = Preferences.NewTabPage.showNewTabFavourites

  var body: some View {
    Form {
      Section {
        Toggle(Strings.Widgets.favoritesWidgetTitle, isOn: $showNewTabFavourites.value)
      } header: {
        Text(Strings.Widgets.widgetTitle)
      }
    }
    .tint(Color(braveSystemName: .primary40))
    .navigationTitle(Strings.NTP.settingsTitle)
    .navigationBarTitleDisplayMode(.inline)
  }

}

class NTPTableViewController: UIHostingController<NewTabPageSettingsView> {
  init() {
    super.init(rootView: .init())
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
