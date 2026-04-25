// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct WalletActionsGridBackgroundView: View {
  @Environment(\.sizeCategory) private var sizeCategory

  var backgroundColor: Color = Color(.secondaryBraveGroupedBackground)

  private var backgroundShape: some InsettableShape {
    RoundedRectangle(cornerRadius: 10, style: .continuous)
  }

  var body: some View {
    backgroundShape
      .fill(backgroundColor)
      .overlay(
        // When using an accessibility font size, inset grouped tables automatically change to
        // grouped tables with separators. So we will match this change and add a border around
        // the buttons to make them appear more uniform with the table
        Group {
          if sizeCategory.isAccessibilityCategory {
            backgroundShape
              .strokeBorder(Color(.separator))
          }
        }
      )
  }
}
