// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

struct CircularCapacityGaugeStyle: GaugeStyle {
  var lineWidth: CGFloat

  func makeBody(configuration: Configuration) -> some View {
    Color.clear
      .overlay {
        Circle()
          .inset(by: lineWidth / 2)
          .stroke(lineWidth: lineWidth)
          .foregroundStyle(.tertiary)
      }
      .overlay {
        Circle()
          .inset(by: lineWidth / 2)
          .rotation(.degrees(-90))
          .trim(from: 0, to: configuration.value)
          .stroke(style: .init(lineWidth: lineWidth, lineCap: .round, lineJoin: .round))
          .foregroundStyle(.primary)
          .animation(
            .spring(response: 0.3, dampingFraction: 0.8),
            value: configuration.value
          )
      }
  }
}

extension GaugeStyle where Self == CircularCapacityGaugeStyle {
  /// A gauge style that displays a closed ring that's partially filled in to
  /// indicate the gauge's current value with a line width set to 2 points.
  ///
  /// This matches the `accessoryCircularCapacity` gauge style but is not a fixed size
  ///
  /// This GaugeStyle currently ignores any label associated with the view
  static var circularCapacity: CircularCapacityGaugeStyle { .init(lineWidth: 2) }

  /// A gauge style that displays a closed ring that's partially filled in to
  /// indicate the gauge's current value.
  ///
  /// This matches the `accessoryCircularCapacity` gauge style but is not a fixed size
  ///
  /// This GaugeStyle currently ignores any label associated with the view
  static func circularCapacity(lineWidth: CGFloat) -> CircularCapacityGaugeStyle {
    .init(lineWidth: lineWidth)
  }
}

#if DEBUG
// swift-format-ignore
#Preview {
  HStack {
    Gauge(value: 0) { Text(verbatim: "0%") }
    Gauge(value: 0.5) { Text(verbatim: "50%") }
    Gauge(value: 1) { Text(verbatim: "100%") }
  }
  .gaugeStyle(.circularCapacity(lineWidth: 10))
  .padding()
}
#endif
