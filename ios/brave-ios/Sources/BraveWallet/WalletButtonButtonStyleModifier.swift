// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct WalletButtonStyleModifier: ViewModifier {
  func body(content: Content) -> some View {
    if #available(iOS 18, *) {
      content.buttonStyle(_OS18ButtonStyle())
    } else {
      content.buttonStyle(_StandardButtonStyle())
    }
  }

  @available(
    iOS,
    introduced: 18.0,
    message: """
      Similar to MenuRowButtonStyleModifier. This is a custom PrimitiveButtonStyle for a
      plain button.

      On iOS 18 there is a bug where a Button inside of a ScrollView which is being presented
      in a sheet will not cancel their tap gesture when a drag occurs which would move the sheet
      instead of the ScrollView, and as a result will execute the action you started your drag on
      even if you dismiss the sheet.

      This is tested up to iOS 18.2 and still broken.
      """
  )
  private struct _OS18ButtonStyle: PrimitiveButtonStyle {
    @GestureState private var isPressed: Bool = false

    func makeBody(configuration: Configuration) -> some View {
      configuration.label
        .simultaneousGesture(
          DragGesture(minimumDistance: 0).updating(
            $isPressed,
            body: { _, state, _ in state = true }
          )
        )
        .simultaneousGesture(
          TapGesture().onEnded {
            configuration.trigger()
          }
        )
        .hoverEffect()
        .background(
          Color(braveSystemName: .iosBrowserContainerHighlightIos).opacity(isPressed ? 1 : 0)
            .animation(isPressed ? nil : .default, value: isPressed)
        )
    }
  }

  private struct _StandardButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
      configuration.label
        .hoverEffect()
        .background(
          Color(braveSystemName: .iosBrowserContainerHighlightIos).opacity(
            configuration.isPressed ? 1 : 0
          )
        )
    }
  }
}
