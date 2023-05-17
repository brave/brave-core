// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import DesignSystem

struct ShieldToggleView: View {
  typealias OnChangeCallback = (Bool) -> Void
  let title: String
  var subtitle: String?
  @Binding var toggle: Bool
  let onChange: OnChangeCallback?
  
  init(title: String, subtitle: String?, toggle: Binding<Bool>, onChange: OnChangeCallback? = nil) {
    self.title = title
    self.subtitle = subtitle
    _toggle = toggle
    self.onChange = onChange
  }
  
  var body: some View {
    Toggle(isOn: $toggle) {
      ShieldLabelView(title: title, subtitle: subtitle)
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
      .toggleStyle(SwitchToggleStyle(tint: .accentColor))
      .onChange(of: toggle) { newValue in
        onChange?(newValue)
      }
  }
}
