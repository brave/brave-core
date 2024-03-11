// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import SwiftUI

struct AIChatProductIcon<S: Shape>: View {

  var containerShape: S
  var padding: CGFloat

  var body: some View {
    Image(braveSystemName: "leo.product.brave-leo")
      .foregroundColor(.white)
      .padding(padding)
      .background(
        LinearGradient(braveSystemName: .iconsActive),
        in: containerShape
      )
  }
}
