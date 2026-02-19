// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import SwiftUI

struct ManagedPasswordRowLabelStyle: LabelStyle {
  enum Context {
    case select
    case unselect
    case plain
  }

  var context: Context

  func makeBody(configuration: Configuration) -> some View {
    HStack(spacing: 12) {
      switch context {
      case .select:
        Image(braveSystemName: "leo.check.circle-filled")
          .font(.title2)
          .foregroundColor(Color(.braveBlurpleTint))
      case .unselect:
        Image(braveSystemName: "leo.radio.unchecked")
          .font(.title2)
          .foregroundColor(Color(.secondaryBraveLabel))
      case .plain:
        EmptyView()
      }
      configuration.icon
      configuration.title
      Spacer()
    }
  }
}
