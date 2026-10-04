// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// A button style applies a stroke border outline, padding and text styling
///
/// In Nala, this matches the "Outline" button component
public struct OutlineButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .modifier(ButtonLabelModifier())
      .foregroundStyle(Color(braveSystemName: isEnabled ? .textInteractive : .textDisabled))
      .background {
        if configuration.isPressed {
          Color(braveSystemName: .buttonBackground).opacity(0.1)
        }
      }
      .background(
        Capsule()
          .strokeBorder(
            Color(braveSystemName: isEnabled ? .dividerInteractive : .buttonDisabled),
            lineWidth: 1
          )
      )
      .clipShape(.capsule)
      .contentShape(.capsule)
      .hoverEffect()
      .animation(.linear(duration: 0.15), value: isEnabled)
  }
}

extension ButtonStyle where Self == OutlineButtonStyle {
  public static var outline: Self { .init() }
}
