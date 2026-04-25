// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import SwiftUI

struct PopoverWrapperView<Content>: View & PopoverContentComponent where Content: View {

  var backgroundColor: UIColor
  var content: Content

  init(backgroundColor: UIColor, @ViewBuilder content: () -> Content) {
    self.backgroundColor = backgroundColor
    self.content = content()
  }

  var body: some View {
    content
  }

  var popoverBackgroundColor: UIColor {
    backgroundColor
  }
}
