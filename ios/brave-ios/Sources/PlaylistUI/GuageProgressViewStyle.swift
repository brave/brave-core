// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

struct GuageProgressViewStyle: ProgressViewStyle {
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
          .trim(from: 0, to: configuration.fractionCompleted ?? 0)
          .stroke(style: .init(lineWidth: lineWidth, lineCap: .round, lineJoin: .round))
          .foregroundStyle(.primary)
          .animation(
            .spring(response: 0.3, dampingFraction: 0.8),
            value: configuration.fractionCompleted
          )
      }
  }
}

extension ProgressViewStyle where Self == GuageProgressViewStyle {
  /// Shows progress in the form of a circular Guage similar to the watchOS style with a line width
  /// set to 2 points.
  ///
  /// This ProgressViewStyle currently ignores any label associated with the view
  static var guage: GuageProgressViewStyle { .init(lineWidth: 2) }

  /// Shows progress in the form of a circular Guage similar to the watchOS style
  ///
  /// This ProgressViewStyle currently ignores any label associated with the view
  static func guage(lineWidth: CGFloat) -> GuageProgressViewStyle { .init(lineWidth: lineWidth) }
}

#if DEBUG
#Preview {
  HStack {
    ProgressView("", value: 0, total: 1)
    ProgressView("", value: 0.5, total: 1)
    ProgressView("", value: 1, total: 1)
  }
  .progressViewStyle(.guage(lineWidth: 10))
  .padding()
}
#endif
