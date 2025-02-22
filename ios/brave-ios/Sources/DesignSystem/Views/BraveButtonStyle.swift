// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

public struct BraveButtonSize {
  public var font: Font
  public var padding: EdgeInsets
  public var radius: CGFloat
  public var minHeight: CGFloat

  public init(
    font: Font,
    padding: EdgeInsets,
    radius: CGFloat,
    minHeight: CGFloat
  ) {
    self.font = font
    self.padding = padding
    self.radius = radius
    self.minHeight = minHeight
  }

  public static let small: Self = .init(
    font: Font.caption.weight(.semibold),
    padding: .init(top: 8, leading: 8, bottom: 8, trailing: 8),
    radius: 8,
    minHeight: 20
  )
  public static let normal: Self = .init(
    font: Font.callout.weight(.semibold),
    padding: .init(top: 12, leading: 12, bottom: 12, trailing: 12),
    radius: 12,
    minHeight: 20
  )
  public static let large: Self = .init(
    font: Font.body.weight(.semibold),
    padding: .init(top: 12, leading: 16, bottom: 12, trailing: 16),
    radius: 16,
    minHeight: 28
  )
}

public struct BraveFilledButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled

  public var size: BraveButtonSize

  public init(size: BraveButtonSize) {
    self.size = size
  }

  public func makeBody(configuration: Configuration) -> some View {
    let clipShape = RoundedRectangle(cornerRadius: size.radius, style: .continuous)
    configuration.label
      .frame(minHeight: size.minHeight)
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
      .multilineTextAlignment(.center)
  }
}

public struct BraveOutlineButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled

  public var size: BraveButtonSize

  public init(
    size: BraveButtonSize
  ) {
    self.size = size
  }

  public func makeBody(configuration: Configuration) -> some View {
    let clipShape = RoundedRectangle(cornerRadius: size.radius, style: .continuous)
    configuration.label
      .frame(minHeight: size.minHeight)
      .opacity(configuration.isPressed ? 0.7 : 1.0)
      .font(size.font)
      .foregroundColor(
        isEnabled ? Color(braveSystemName: .textInteractive) : Color(braveSystemName: .textDisabled)
      )
      .padding(size.padding)
      .background(
        Group {
          if isEnabled {
            Color(braveSystemName: .dividerInteractive).opacity(configuration.isPressed ? 0.7 : 1.0)
          } else {
            Color(braveSystemName: .buttonDisabled)
          }
        }
        .clipShape(clipShape.inset(by: 0.5).stroke())
      )
      .clipShape(clipShape)
      .contentShape(clipShape)
      .animation(.linear(duration: 0.15), value: isEnabled)
      .multilineTextAlignment(.center)
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
                HStack(spacing: 2) {
                  Image(braveSystemName: "leo.check.normal")
                  Text(verbatim: "Text")
                }
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
