// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

extension AnyTransition {
  static var playButtonTransition: AnyTransition {
    .scale.combined(with: .opacity).animation(.spring(response: 0.3, dampingFraction: 0.7))
  }
}

/// A special button style that applies to components shown in playback controls in playlist.
///
/// Playback controls can be sized with 3 different size modes (regular, large, and extra large)
/// which will affect the overall button size, image scale and font style used to size SF Symbols.
///
/// Use the `.tint` modifier to apply the color that should be used for the icon and use
/// `.backgroundStyle` modifier to choose which color should be used when the user is hovering over
/// the button with a pointer device or the user is currently touching the button
///
/// - Note: This button style sets `labelStyle` to `iconOnly` and thus will not display the text
///         component of any `Label` provided (though it will still be read aloud in VoiceOver)
struct PlaybackControlButtonStyle: ButtonStyle {
  enum Size {
    case regular
    case large
    case extraLarge
  }
  var size: Size

  private let scale: CGFloat = 0.85
  @State private var isPressed: Bool = false
  @State private var pressDownTime: Date?
  @State private var delayedTouchUpTask: Task<Void, Error>?
  @Environment(\.isEnabled) private var isEnabled

  private var length: CGFloat {
    switch size {
    case .regular:
      return 24
    case .large:
      return 36
    case .extraLarge:
      return 48
    }
  }

  private var imageScale: Image.Scale {
    switch size {
    case .regular:
      return .medium
    case .large, .extraLarge:
      return .large
    }
  }

  private var fontStyle: Font.TextStyle {
    switch size {
    case .regular:
      return .title3
    case .large:
      return .title2
    case .extraLarge:
      return .largeTitle
    }
  }

  @ScaledMetric private var multiplier = 1.0

  func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .labelStyle(.iconOnly)
      .imageScale(imageScale)
      .font(.system(fontStyle))
      .frame(height: multiplier * length)
      .contentShape(.rect)
      .foregroundStyle(.tint)
      .background {
        if isPressed {
          RoundedRectangle(cornerRadius: 8, style: .continuous)
            .foregroundStyle(.background)
            // May want to reconsider this later and just add 8pt padding to button itself
            .padding(-8)
            .transition(
              .asymmetric(
                insertion: .opacity.animation(.linear(duration: 0.05)),
                removal: .opacity.animation(.interactiveSpring())
              )
            )
        }
      }
      .scaleEffect(isPressed ? scale : 1.0)
      .opacity(isPressed ? 0.95 : 1.0)
      .onChange(
        of: configuration.isPressed,
        perform: { value in
          // Makes it so the "pressed" state shows more of its animation if you tap and immediately
          // lift your finger
          if value {
            isPressed = value
            pressDownTime = .now
            delayedTouchUpTask?.cancel()
          } else {
            if let pressDownTime, case let delta = Date.now.timeIntervalSince(pressDownTime),
              delta < 0.1
            {
              delayedTouchUpTask = Task { @MainActor in
                try await Task.sleep(nanoseconds: NSEC_PER_MSEC * UInt64((0.1 - delta) * 1000))
                isPressed = value
              }
            } else {
              isPressed = value
            }
          }
        }
      )
      .animation(.spring(response: 0.3, dampingFraction: 0.8), value: isPressed)
      .opacity(isEnabled ? 1 : 0.5)
      .animation(.linear(duration: 0.1), value: isEnabled)
  }
}

extension ButtonStyle where Self == PlaybackControlButtonStyle {
  static var playbackControl: PlaybackControlButtonStyle {
    .init(size: .regular)
  }
  static func playbackControl(
    size: PlaybackControlButtonStyle.Size
  ) -> PlaybackControlButtonStyle {
    .init(size: size)
  }
}

#if DEBUG
struct PlaybackControlButtonStyle_PreviewProvider: PreviewProvider {
  static var previews: some View {
    VStack(spacing: 0) {
      HStack {
        Button {
        } label: {
          Label("Hello, World", braveSystemImage: "leo.1x")
        }
        .padding()
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        Button {
        } label: {
          Label("Hello, World", braveSystemImage: "leo.1x")
        }
        .padding()
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .colorScheme(.dark)
        .background(Color.black)
      }
      Divider()
      HStack {
        Button {
        } label: {
          HStack {
            Label("Hello, World", braveSystemImage: "leo.2x")
            Text(verbatim: "Test")
          }
        }
        .padding()
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        Button {
        } label: {
          Label("Hello, World", braveSystemImage: "leo.2x")
        }
        .padding()
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .colorScheme(.dark)
        .background(Color.black)
      }
    }
    .buttonStyle(.playbackControl)
    .backgroundStyle(.tertiary)
  }
}
#endif
