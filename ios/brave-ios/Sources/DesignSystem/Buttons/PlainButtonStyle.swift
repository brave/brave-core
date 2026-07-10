// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// A button style that applies a more subtle button background and standard padding & text styling.
///
/// In Nala, this matches the "Plain" button component
public struct PlainBorderedButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled
  private let shape: BraveButtonShape

  public init(shape: BraveButtonShape = .capsule) {
    self.shape = shape
  }

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .braveButtonLabel(shape: shape)
      .foregroundStyle(Color(braveSystemName: isEnabled ? .textInteractive : .textDisabled))
      .background {
        if isEnabled {
          Color(braveSystemName: .buttonBackground)
            .opacity(0.05)
            .mix(
              with: Color(braveSystemName: .fixedForeground),
              by: configuration.isPressed ? 0.1 : 0
            )
        } else {
          // 7% icon default is currently the background color used in Figma for this button styles
          // disabled state
          Color(braveSystemName: .iconDefault).opacity(0.07)
        }
      }
      .braveButtonClipShape(shape)
      .hoverEffect()
      .animation(.linear(duration: 0.15), value: isEnabled)
  }
}

/// A button style that matches a Glass button style but with Nala standard padding & text styling
@available(iOS 26.0, *)
public struct PlainGlassButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled
  private let shape: BraveButtonShape

  public init(shape: BraveButtonShape = .capsule) {
    self.shape = shape
  }

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .braveButtonLabel(shape: shape)
      // Glass buttons use a specific set of colours because text color adjusts dynamically based
      // on content behind the button and disabled status matches Apple
      .foregroundStyle(isEnabled ? .primary : .tertiary)
      .braveGlassEffect(.regular.interactive(isEnabled), shape: shape)
  }
}

extension ButtonStyle where Self == PlainBorderedButtonStyle {
  public static var plainBordered: Self { .init() }
  public static var plainBorderedCircle: Self { .init(shape: .circle) }
}

@available(iOS 26.0, *)
extension ButtonStyle where Self == PlainGlassButtonStyle {
  public static var plainGlass: Self { .init() }
  public static var plainGlassCircle: Self { .init(shape: .circle) }
}
