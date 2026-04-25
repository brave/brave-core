// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// A button style that mimics a table cell being highlighted in the background
public struct TableCellButtonStyle: ButtonStyle {
  @Environment(\.colorScheme) private var colorScheme: ColorScheme

  public init() {}
  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .contentShape(Rectangle())  // Needed or taps don't activate on empty space
      .background(
        Color(colorScheme == .dark ? .white : .black)
          .opacity(configuration.isPressed ? 0.1 : 0.0)
      )
      .hoverEffect(.highlight)
  }
}
