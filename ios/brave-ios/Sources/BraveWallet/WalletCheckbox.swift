// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct WalletCheckbox: View {
  @Binding var isChecked: Bool
  let colorOverride: UIColor?

  init(
    isChecked: Binding<Bool>,
    colorOverride: UIColor? = nil
  ) {
    self._isChecked = isChecked
    self.colorOverride = colorOverride
  }

  var body: some View {
    Button {
      isChecked.toggle()
    } label: {
      Image(braveSystemName: isChecked ? "leo.checkbox.checked" : "leo.checkbox.unchecked")
        .renderingMode(.template)
        .foregroundColor(Color(colorOverride ?? (isChecked ? .braveBlurpleTint : .braveDisabled)))
    }
  }
}
