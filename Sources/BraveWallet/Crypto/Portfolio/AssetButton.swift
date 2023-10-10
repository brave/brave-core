/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import DesignSystem

struct AssetButton: View {
  
  let braveSystemName: String
  let action: () -> Void
  
  @ScaledMetric var length = 36
  
  var body: some View {
    Button(action: action) {
      Image(braveSystemName: braveSystemName)
        .foregroundColor(Color(braveSystemName: .iconInteractive))
        .imageScale(.medium)
        .padding(6)
        .frame(width: length, height: length)
        .background(
          Circle()
            .strokeBorder(Color(braveSystemName: .dividerInteractive), lineWidth: 1)
        )
    }
  }
}
