// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import SwiftUI

public struct OptionToggleView: View {
  let title: String
  var subtitle: String?
  @ObservedObject var option: Preferences.Option<Bool>
  let onChange: ToggleView.OnChangeCallback?

  public init(
    title: String,
    subtitle: String? = nil,
    option: Preferences.Option<Bool>,
    onChange: ToggleView.OnChangeCallback? = nil
  ) {
    self.title = title
    self.subtitle = subtitle
    self.option = option
    self.onChange = onChange
  }

  public var body: some View {
    ToggleView(
      title: title,
      subtitle: subtitle,
      toggle: $option.value,
      onChange: onChange
    )
  }
}
