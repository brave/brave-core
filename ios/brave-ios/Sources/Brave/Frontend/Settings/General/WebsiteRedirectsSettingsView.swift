// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Preferences
import Shared
import SwiftUI

struct WebsiteRedirectsSettingsView: View {
  @ObservedObject private var reddit = Preferences.WebsiteRedirects.reddit
  @ObservedObject private var npr = Preferences.WebsiteRedirects.npr

  var body: some View {
    Form {
      Section {
        Toggle(isOn: $reddit.value) {
          Text("reddit.com \(Image(systemName: "arrow.right")) old.reddit.com")
        }
        .toggleStyle(SwitchToggleStyle(tint: .accentColor))
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.redditRedirectFooter)
      }

      Section {
        Toggle(isOn: $npr.value) {
          Text("npr.org \(Image(systemName: "arrow.right")) text.npr.org")
        }
        .toggleStyle(SwitchToggleStyle(tint: .accentColor))
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.nprRedirectFooter)
      }
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle(Strings.urlRedirectsSettings)
  }
}

#if DEBUG
struct URLRedirectionSettingsView_Previews: PreviewProvider {
  static var previews: some View {
    WebsiteRedirectsSettingsView()
  }
}
#endif
