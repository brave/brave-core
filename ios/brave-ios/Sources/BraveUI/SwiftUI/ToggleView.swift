// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

public struct ToggleView: View {
  public typealias OnChangeCallback = (Bool) -> Void

  let title: String
  var subtitle: String?
  @Binding var toggle: Bool
  let onChange: OnChangeCallback?

  public init(
    title: String,
    subtitle: String? = nil,
    toggle: Binding<Bool>,
    onChange: OnChangeCallback? = nil
  ) {
    self.title = title
    self.subtitle = subtitle
    _toggle = toggle
    self.onChange = onChange
  }

  public var body: some View {
    Toggle(isOn: $toggle) {
      LabelView(title: title, subtitle: subtitle)
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
    .toggleStyle(SwitchToggleStyle(tint: .accentColor))
    .onChange(of: toggle) { newValue in
      onChange?(newValue)
    }
  }
}
