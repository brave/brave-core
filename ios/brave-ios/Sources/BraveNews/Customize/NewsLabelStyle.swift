// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveUI
import SwiftUI

/// A basic label style that is used to remove the firstTextBaseline content alignment and
/// also applies a separator alignment guide on iOS 16
struct NewsLabelStyle: LabelStyle {
  func makeBody(configuration: Configuration) -> some View {
    HStack(spacing: 10) {
      configuration.icon
      configuration.title
    }
    .osAvailabilityModifiers { content in
      if #available(iOS 16.0, *) {
        content.alignmentGuide(.listRowSeparatorLeading) { d in
          d[.leading]
        }
      } else {
        content
      }
    }
  }
}
