// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// A button style that applies some standard text styling & padding but otherwise doesn't decorate
/// its content while idle, but applies a visual effect to indicate the pressed or enabled state of
/// the button.
///
/// In Nala, this matches the "Plain Faint" button component
public struct PlainFaintButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .modifier(ButtonLabelModifier())
      .foregroundStyle(Color(braveSystemName: isEnabled ? .textSecondary : .textDisabled))
      .background {
        if configuration.isPressed {
          // 7% icon default is currently the background color used in Figma for this button styles
          // highlighted state
          Color(braveSystemName: .iconDefault).opacity(0.07)
        }
      }
      .clipShape(.capsule)
      .contentShape(.capsule)
      .hoverEffect()
      .animation(.linear(duration: 0.15), value: isEnabled)
  }
}

extension ButtonStyle where Self == PlainFaintButtonStyle {
  public static var plainFaint: Self { .init() }
}
