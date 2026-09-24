// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

// The base set of modifiers used for all Brave styled buttons based on the control size
struct ButtonLabelModifier: ViewModifier {
  @Environment(\.controlSize) private var controlSize

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

  func body(content: Content) -> some View {
    content
      .labelStyle(BraveSystemTitleAndIconLabelStyle())
      // These base values need to be applied directly to the Button's label because
      // its possible for the Button label to be a simple `Text` view
      .padding(basePadding(for: controlSize))
      .font(font)
      .fontWeight(.semibold)
      .multilineTextAlignment(.center)
  }
}

// The default label style for Brave buttons, which applies the correct spacing and sizes the
// icon correctly based on the control size
public struct BraveSystemTitleAndIconLabelStyle: LabelStyle {
  public func makeBody(configuration: Configuration) -> some View {
    HStack(spacing: 4) {
      configuration.icon
        .modifier(BoundedSystemIconModifier())
      configuration.title
    }
    .accessibilityElement()
  }
}

extension LabelStyle where Self == BraveSystemTitleAndIconLabelStyle {
  public static var buttonTitleAndIcon: Self { .init() }
}

// An icon only style to use with Brave buttons instead of `iconOnly`. Correctly adjusts padding
// and sizes the icon correctly based on the control size
public struct BraveSystemIconOnlyLabelStyle: LabelStyle {
  @Environment(\.controlSize) private var controlSize

  private var paddingAdjustment: EdgeInsets {
    let base = basePadding(for: controlSize)
    // The base padding includes additional horizontal padding that needs to be removed for
    // icon only label styles
    let horizontalAdjustment = base.top - base.leading
    return .init(horizontal: horizontalAdjustment, vertical: 0)
  }

  public func makeBody(configuration: Configuration) -> some View {
    configuration.icon
      .modifier(BoundedSystemIconModifier())
      .padding(paddingAdjustment)
      .accessibilityRepresentation {
        configuration.title
      }
  }
}

extension LabelStyle where Self == BraveSystemIconOnlyLabelStyle {
  public static var buttonIconOnly: Self { .init() }
}

// Modifies the Label's icon to size Nala icons approriately for Nala button styles
private struct BoundedSystemIconModifier: ViewModifier {
  @Environment(\.controlSize) private var controlSize
  @ScaledMetric private var scale = 1.0

  private var bounds: Double {
    switch controlSize {
    case .mini: return 16
    case .small: return 18
    case .regular: return 20
    case .large: return 24
    case .extraLarge: return 28
    @unknown default:
      return 20
    }
  }

  func body(content: Content) -> some View {
    content
      .font(.system(size: (bounds - 3) * scale))
      .frame(width: bounds * scale, height: bounds * scale)
  }
}

private func basePadding(for controlSize: ControlSize) -> EdgeInsets {
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

extension EdgeInsets {
  fileprivate init(horizontal: Double, vertical: Double) {
    self.init(top: vertical, leading: horizontal, bottom: vertical, trailing: horizontal)
  }
}
