// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveWidgetsModels
import DesignSystem
import Preferences
import Strings
import SwiftUI

struct ShortcutButtonPickerView: View {
  @ObservedObject private var selectedShortcut = Preferences.General.toolbarShortcutButton
  @Environment(\.dismiss) var dismiss

  var body: some View {
    Form {
      Picker("", selection: $selectedShortcut.value) {
        Label(Strings.ShortcutButton.hideButtonTitle, braveSystemImage: "leo.eye.off")
          .tag(Int?.none)
        ForEach(WidgetShortcut.eligibleButtonShortcuts, id: \.self) { shortcut in
          Label(shortcut.displayString, braveSystemImage: shortcut.braveSystemImageName ?? "")
            .tag(Int?.some(shortcut.rawValue))
        }
      }
      .pickerStyle(.inline)
      .tint(Color(braveSystemName: .iconDefault))
      .foregroundStyle(Color(braveSystemName: .textPrimary))
      .listRowBackground(Color(uiColor: .secondaryBraveGroupedBackground))
    }
    .onChange(of: selectedShortcut.value) { newValue in
      dismiss()
    }
    .navigationTitle(Strings.ShortcutButton.shortcutButtonTitle)
    .scrollContentBackground(.hidden)
    .background(Color(uiColor: .braveGroupedBackground))
  }
}
