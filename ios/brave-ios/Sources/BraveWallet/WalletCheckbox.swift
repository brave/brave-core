// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct CheckboxToggleStyle: ToggleStyle {
  @Environment(\.isEnabled) private var isEnabled

  func makeBody(configuration: Configuration) -> some View {
    Button {
      configuration.isOn.toggle()
    } label: {
      HStack {
        Image(
          braveSystemName: configuration.isOn ? "leo.checkbox.checked" : "leo.checkbox.unchecked"
        )
        .foregroundStyle(
          Color(
            braveSystemName: (isEnabled
              ? (configuration.isOn ? .buttonBackground : .buttonDisabled) : .buttonDisabled)
          )
        )
        configuration.label
      }
    }
    .buttonStyle(.plain)
  }
}
