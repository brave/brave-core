// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// A button style applies a hero gradient background, padding and text styling
///
/// In Nala, this matches the "Hero" button component
public struct HeroButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .modifier(ButtonLabelModifier())
      .foregroundStyle(isEnabled ? .white : Color(braveSystemName: .textDisabled))
      .background {
        if isEnabled {
          LinearGradient(braveSystemName: .hero)
            .overlay {
              Color(braveSystemName: .fixedForeground)
                .opacity(configuration.isPressed ? 0.2 : 0)
            }
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

/// A button style applies standard padding and text styling on a tinted glass effect background
@available(iOS 26.0, *)
public struct GlassHeroButtonStyle: ButtonStyle {
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
              ? Color(braveSystemName: .primitiveBrandsRorange2)
              : Color(uiColor: .tertiarySystemFill)
          )
          .interactive(isEnabled),
        in: .capsule
      )
  }
}

extension ButtonStyle where Self == HeroButtonStyle {
  public static var hero: Self { .init() }
}

@available(iOS 26.0, *)
extension ButtonStyle where Self == GlassHeroButtonStyle {
  public static var glassHero: Self { .init() }
}
