// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

// The base set of modifiers used for all Brave styled buttons based on the control size
struct ButtonLabelModifier: ViewModifier {
  @Environment(\.controlSize) private var controlSize

  private var padding: EdgeInsets {
    switch controlSize {
    case .mini: return .init(horizontal: 8, vertical: 6)
    case .small: return .init(horizontal: 12, vertical: 9)
    case .regular: return .init(horizontal: 12, vertical: 12)
    case .large: return .init(horizontal: 16, vertical: 14)
    case .extraLarge: return .init(horizontal: 16, vertical: 16)
    @unknown default:
      return .init(horizontal: 12, vertical: 12)
    }
  }

  private var font: Font {
    switch controlSize {
    case .mini: return .caption
    case .small: return .caption
    case .regular: return .subheadline
    case .large: return .callout
    case .extraLarge: return .headline
    @unknown default:
      return .subheadline
    }
  }

  private var minimumSize: Double {
    switch controlSize {
    case .mini: return 28
    case .small: return 36
    case .regular: return 44
    case .large: return 52
    case .extraLarge: return 60
    @unknown default:
      return 44
    }
  }

  func body(content: Content) -> some View {
    content
      .labelStyle(BaseLabelStyle())
      .font(font)
      .fontWeight(.semibold)
      .padding(padding)
      .frame(minWidth: minimumSize, minHeight: minimumSize)
      .multilineTextAlignment(.center)
  }

  private struct BaseLabelStyle: LabelStyle {
    func makeBody(configuration: Configuration) -> some View {
      if #available(iOS 26.0, *) {
        Label(configuration)
          .labelIconToTitleSpacing(4)
      } else {
        HStack(spacing: 4) {
          configuration.icon
          configuration.title
        }
      }
    }
  }
}

extension EdgeInsets {
  fileprivate init(horizontal: Double, vertical: Double) {
    self.init(top: vertical, leading: horizontal, bottom: vertical, trailing: horizontal)
  }
}
