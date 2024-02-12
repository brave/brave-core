// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Preferences

struct OptionToggleView: View {
  let title: String
  var subtitle: String?
  @ObservedObject var option: Preferences.Option<Bool>
  let onChange: ShieldToggleView.OnChangeCallback?
  
  init(title: String, subtitle: String?,
       option: Preferences.Option<Bool>, onChange: ShieldToggleView.OnChangeCallback? = nil) {
    self.title = title
    self.subtitle = subtitle
    self.option = option
    self.onChange = onChange
  }
  
  var body: some View {
    ShieldToggleView(
      title: title,
      subtitle: subtitle,
      toggle: $option.value,
      onChange: onChange
    )
  }
}
