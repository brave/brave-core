// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// A button style applies a standard background, padding and text styling
///
/// In Nala, this matches the "Filled" button component
public struct FilledButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .modifier(ButtonLabelModifier())
      .foregroundStyle(Color(braveSystemName: isEnabled ? .schemesOnPrimary : .textDisabled))
      .background {
        if isEnabled {
          Color(braveSystemName: .buttonBackground)
            .mix(
              with: Color(braveSystemName: .fixedForeground),
              by: configuration.isPressed ? 0.2 : 0
            )
        } else {
          Color(braveSystemName: .buttonDisabled)
        }
      }
      .clipShape(.capsule)
      .contentShape(.capsule)
      .hoverEffect()
      .animation(.linear(duration: 0.15), value: isEnabled)
  }
}

/// A button style applies standard padding and text styling on a tinted glass effect background,
@available(iOS 26.0, *)
public struct GlassFilledButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .modifier(ButtonLabelModifier())
      // Glass buttons use a specific set of colours because text color adjusts dynamically based
      // on content behind the button and disabled status matches Apple
      .foregroundStyle(isEnabled ? AnyShapeStyle(.white) : AnyShapeStyle(.tertiary))
      .glassEffect(
        .regular
          .tint(
            isEnabled
              ? Color(braveSystemName: .primitivePrimary40) : Color(uiColor: .tertiarySystemFill)
          )
          .interactive(isEnabled),
        in: .capsule
      )
  }
}

extension ButtonStyle where Self == FilledButtonStyle {
  public static var filled: Self { .init() }
}

@available(iOS 26.0, *)
extension ButtonStyle where Self == GlassFilledButtonStyle {
  public static var glassFilled: Self { .init() }
}
