// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
import UIKit
import struct SwiftUI.Angle

public struct BraveGradient {
  public struct Stop {
    var color: UIColor
    var position: Double
  }
  var type: CAGradientLayerType = .axial
  var stops: [Stop]
  var startPoint: CGPoint
  var endPoint: CGPoint
  
  init(stops: [Stop], angle: Angle) {
    let alpha = angle.radians
    let startPoint = CGPoint(
      x: 0.5 * sin(alpha) + 0.5,
      y: -0.5 * cos(alpha) + 0.5
    )
    let endPoint = CGPoint(
      x: -0.5 * sin(alpha) + 0.5,
      y: 0.5 * cos(alpha) + 0.5
    )
    self.init(stops: stops, startPoint: startPoint, endPoint: endPoint)
  }
  
  init(stops: [Stop], startPoint: CGPoint, endPoint: CGPoint) {
    self.stops = stops
    self.startPoint = startPoint
    self.endPoint = endPoint
  }
}

extension Angle {
  /// Create an Angle using degrees reported by Figma
  ///
  /// CSS gradients are flipped, therefore need to adjusted to look correct while in iOS
  static func figmaDegrees(_ degrees: Double) -> Self {
    self.init(degrees: degrees + 180.0)
  }
}

extension BraveGradient {
  public static var lightGradient02: BraveGradient {
    .init(
      stops: [
        .init(color: DesignSystemColor.gradient02_step0.color, position: 0),
        .init(color: DesignSystemColor.gradient02_step1.color, position: 0.56),
        .init(color: DesignSystemColor.gradient02_step2.color, position: 1.0),
      ],
      angle: .figmaDegrees(122.5)
    )
  }
  public static var darkGradient02: BraveGradient {
    .init(
      stops: [
        .init(color: DesignSystemColor.gradient02_step0.color, position: 0.06),
        .init(color: DesignSystemColor.gradient02_step1.color, position: 0.44),
        .init(color: DesignSystemColor.gradient02_step2.color, position: 1.0),
      ],
      angle: .figmaDegrees(314)
    )
  }
  public static var gradient03: BraveGradient {
    .init(
      stops: [
        .init(color: DesignSystemColor.gradient03_step0.color, position: 0),
        .init(color: DesignSystemColor.gradient03_step1.color, position: 0.985),
      ],
      angle: .figmaDegrees(306)
    )
  }
}

extension BraveGradientView {
  public static var gradient02: BraveGradientView {
    .init { traitCollection in
      if traitCollection.userInterfaceStyle == .dark {
        return .darkGradient02
      }
      return .lightGradient02
    }
  }
  public static var gradient03: BraveGradientView {
    .init(gradient: .gradient03)
  }
}
