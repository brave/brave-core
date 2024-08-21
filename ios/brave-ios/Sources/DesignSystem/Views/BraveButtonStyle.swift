// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

public struct BraveButtonSize {
  public var font: Font
  public var padding: EdgeInsets

  public init(font: Font, padding: EdgeInsets) {
    self.font = font
    self.padding = padding
  }

  public static let small: Self = .init(
    font: Font.caption.weight(.semibold),
    padding: .init(top: 6, leading: 12, bottom: 6, trailing: 12)
  )
  public static let normal: Self = .init(
    font: Font.callout.weight(.semibold),
    padding: .init(top: 8, leading: 14, bottom: 8, trailing: 14)
  )
  public static let large: Self = .init(
    font: Font.body.weight(.semibold),
    padding: .init(top: 10, leading: 20, bottom: 10, trailing: 20)
  )
}

public struct BraveFilledButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled

  private let clipShape = RoundedRectangle(cornerRadius: 48, style: .continuous)
  public var size: BraveButtonSize

  public init(size: BraveButtonSize) {
    self.size = size
  }

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .opacity(configuration.isPressed ? 0.7 : 1.0)
      .font(size.font)
      .foregroundColor(Color(braveSystemName: .schemesOnPrimary))
      .padding(size.padding)
      .background(
        Group {
          if isEnabled {
            Color(braveSystemName: .buttonBackground).opacity(configuration.isPressed ? 0.7 : 1.0)
          } else {
            Color(braveSystemName: .buttonDisabled)
          }
        }
      )
      .clipShape(clipShape)
      .contentShape(clipShape)
      .hoverEffect()
      .animation(.linear(duration: 0.15), value: isEnabled)
  }
}

public struct BraveOutlineButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled

  public var size: BraveButtonSize
  public var enabledTextColor: UIColor
  public var enabledOutlineColor: UIColor

  public init(
    size: BraveButtonSize,
    enabledTextColor: UIColor = .braveLabel,
    enabledOutlineColor: UIColor = .secondaryButtonTint
  ) {
    self.size = size
    self.enabledTextColor = enabledTextColor
    self.enabledOutlineColor = enabledOutlineColor
  }

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .opacity(configuration.isPressed ? 0.7 : 1.0)
      .font(size.font)
      .foregroundColor(isEnabled ? Color(enabledTextColor) : Color(.braveDisabled))
      .padding(size.padding)
      .background(
        Group {
          if isEnabled {
            Color(enabledOutlineColor).opacity(configuration.isPressed ? 0.7 : 1.0)
          } else {
            Color(.braveDisabled)
          }
        }
        .clipShape(Capsule().inset(by: 0.5).stroke())
      )
      .clipShape(Capsule())
      .contentShape(Capsule())
      .animation(.linear(duration: 0.15), value: isEnabled)
  }
}

#if DEBUG
struct BraveButtonStyle_Previews: PreviewProvider {
  static let defaultSizes: [BraveButtonSize] = [
    .small, .normal, .large,
  ]

  static var previews: some View {
    Group {
      HStack {
        ForEach([false, true], id: \.self) { disabled in
          VStack {
            ForEach(defaultSizes.indices, id: \.self) { historyIndex in
              Button {
              } label: {
                Text(verbatim: "Button text")
              }
              .buttonStyle(BraveFilledButtonStyle(size: defaultSizes[historyIndex]))
              .disabled(disabled)
            }
          }
          .padding()
        }
      }
      HStack {
        ForEach([false, true], id: \.self) { disabled in
          VStack {
            ForEach(defaultSizes.indices, id: \.self) { historyIndex in
              Button {
              } label: {
                Text(verbatim: "Button text")
              }
              .buttonStyle(BraveOutlineButtonStyle(size: defaultSizes[historyIndex]))
              .disabled(disabled)
            }
          }
          .padding()
        }
      }
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
