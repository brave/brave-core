// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

/// Applies a scale effect when the user presses down with a spring animation attached to it
public struct SpringButtonStyle: ButtonStyle {
  public var scale: CGFloat = 0.95

  @State private var isPressed: Bool = false
  @State private var pressDownTime: Date?
  @State private var delayedTouchUpTask: Task<Void, Error>?

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .background {
        if isPressed {
          RoundedRectangle(cornerRadius: 8, style: .continuous)
            .fill(.background)
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
      .contentShape(.hoverEffect, .rect(cornerRadius: 8, style: .continuous))
      .hoverEffect()
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
  }
}

extension ButtonStyle where Self == SpringButtonStyle {
  public static var spring: SpringButtonStyle {
    .init()
  }
  public static func spring(
    scale: CGFloat
  ) -> SpringButtonStyle {
    .init(scale: scale)
  }
}

#if DEBUG
struct SpringButtonStyle_PreviewProvider: PreviewProvider {
  static var previews: some View {
    HStack {
      Button("Hello, World") {}
        .padding()
        .frame(maxWidth: .infinity, maxHeight: .infinity)
      Button("Hello, World") {}
        .padding()
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .colorScheme(.dark)
        .background(Color.black)
    }
    .buttonStyle(.spring)
  }
}
#endif
