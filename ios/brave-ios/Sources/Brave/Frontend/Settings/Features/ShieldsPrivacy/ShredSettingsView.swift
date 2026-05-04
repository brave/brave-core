// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import BraveUI
import Strings
import SwiftUI

/// View for displaying global Shred settings
struct ShredSettingsView: View {
  @ObservedObject var settings: AdvancedShieldsSettings

  var body: some View {
    Form {
      Section {
        Picker(selection: $settings.shredLevel) {
          ForEach(SiteShredLevel.allCases) { level in
            LabelView(
              title: level.localizedTitle,
              subtitle: level.localizedDescription
            )
            .tag(level)
          }
        } label: {
          EmptyView()
        }
        .pickerStyle(.inline)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } header: {
        Text(Strings.Shields.autoShredSectionTitle)
      }
      Section {
        ToggleView(
          title: Strings.Shields.shredHistoryRowTitle,
          subtitle: Strings.Shields.shredHistoryRowDescription,
          toggle: $settings.shredHistoryItems
        )
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .scrollContentBackground(.hidden)
    .background(Color(UIColor.braveGroupedBackground))
    .navigationTitle(Strings.Shields.shredSettingsViewTitle)
  }
}
