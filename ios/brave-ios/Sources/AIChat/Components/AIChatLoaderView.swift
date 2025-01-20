// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import SwiftUI

struct AIChatLoaderView: View {
  var body: some View {
    AIChatProductIcon(containerShape: Circle(), padding: 6.0)
      .font(.callout)

    ProgressView()
      .progressViewStyle(BraveProgressStyleDots())
  }
}
