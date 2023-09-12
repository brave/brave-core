// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import UIKit

extension Gradient {
  /// Create a SwiftUI Gradient from a Brave design system gradient
  init(braveSystemName gradient: FigmaGradient) {
    self.init(
      stops: gradient.stops.map { stop in
          .init(color: Color(stop.color), location: CGFloat(stop.position))
      }
    )
  }
}

extension LinearGradient {
  /// Create a SwiftUI LinearGradient from a Brave design system gradient
  public init(braveSystemName gradient: FigmaGradient) {
    assert(gradient.type == .axial, "Attempting to create a LinearGradient with a non-linear Brave defined gradient")
    self.init(
      gradient: Gradient(braveSystemName: gradient),
      startPoint: .init(x: gradient.startPoint.x, y: gradient.startPoint.y),
      endPoint: .init(x: gradient.endPoint.x, y: gradient.endPoint.y)
    )
  }
}

extension GradientView {
  /// Create a GradientView that will render a Brave design system gradient
  public convenience init(braveSystemName gradient: FigmaGradient) {
    self.init(
      colors: gradient.stops.map(\.color),
      positions: gradient.stops.map({ CGFloat($0.position) }),
      startPoint: gradient.startPoint,
      endPoint: gradient.endPoint
    )
  }
}
