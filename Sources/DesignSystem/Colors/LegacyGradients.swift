// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
import UIKit
import SwiftUI

extension Gradient {
  init(braveGradient gradient: BraveGradient) {
    self.init(
      stops: gradient.stops.map { stop in
        .init(color: Color(stop.color), location: CGFloat(stop.position))
      }
    )
  }
}

extension LinearGradient {
  /// Create a SwiftUI LinearGradient from a Brave defined Gradient
  public init(braveGradient gradient: BraveGradient) {
    assert(gradient.type == .axial, "Attempting to create a LinearGradient with a non-linear Brave defined gradient")
    self.init(
      gradient: Gradient(braveGradient: gradient),
      startPoint: .init(x: gradient.startPoint.x, y: gradient.startPoint.y),
      endPoint: .init(x: gradient.endPoint.x, y: gradient.endPoint.y)
    )
  }
}

public struct BraveGradient {
  public struct Stop {
    public var color: UIColor
    public var position: Double
    
    public init(color: UIColor, position: Double) {
      self.color = color
      self.position = position
    }
  }
  public var type: CAGradientLayerType = .axial
  public var stops: [Stop]
  public var startPoint: CGPoint
  public var endPoint: CGPoint
  
  public init(stops: [Stop], angle: Angle) {
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
  
  public init(stops: [Stop], startPoint: CGPoint, endPoint: CGPoint) {
    self.stops = stops
    self.startPoint = startPoint
    self.endPoint = endPoint
  }
}

public extension Angle {
  /// Create an Angle using degrees reported by Figma
  ///
  /// CSS gradients are flipped, therefore need to adjusted to look correct while in iOS
  static func figmaDegrees(_ degrees: Double) -> Self {
    self.init(degrees: degrees + 180.0)
  }
}

extension BraveGradient {
  public static var lightGradient01: BraveGradient {
    .init(
      stops: [
        .init(color: LegacyDesignSystemColor.gradient01_step0.color, position: 0),
        .init(color: LegacyDesignSystemColor.gradient01_step1.color, position: 1.0),
      ],
      angle: .figmaDegrees(126)
    )
  }
  public static var darkGradient01: BraveGradient {
    .init(
      stops: [
        .init(color: LegacyDesignSystemColor.gradient01_step0.color, position: 0),
        .init(color: LegacyDesignSystemColor.gradient01_step1.color, position: 1.0),
      ],
      angle: .figmaDegrees(130)
    )
  }
  public static var lightGradient02: BraveGradient {
    .init(
      stops: [
        .init(color: LegacyDesignSystemColor.gradient02_step0.color, position: 0),
        .init(color: LegacyDesignSystemColor.gradient02_step1.color, position: 0.56),
        .init(color: LegacyDesignSystemColor.gradient02_step2.color, position: 1.0),
      ],
      angle: .figmaDegrees(122.5)
    )
  }
  public static var lightAlternateGradient02: BraveGradient {
    .init(
      stops: [
        .init(color: LegacyDesignSystemColor.gradient02_alt_step0.color, position: 0.16),
        .init(color: LegacyDesignSystemColor.gradient02_alt_step1.color, position: 0.63),
        .init(color: LegacyDesignSystemColor.gradient02_alt_step2.color, position: 1.0),
      ],
      angle: .figmaDegrees(304.5)
    )
  }
  public static var darkGradient02: BraveGradient {
    .init(
      stops: [
        .init(color: LegacyDesignSystemColor.gradient02_step0.color, position: 0.06),
        .init(color: LegacyDesignSystemColor.gradient02_step1.color, position: 0.44),
        .init(color: LegacyDesignSystemColor.gradient02_step2.color, position: 1.0),
      ],
      angle: .figmaDegrees(314)
    )
  }
  public static var darkAlternateGradient02: BraveGradient {
    .init(
      stops: [
        .init(color: LegacyDesignSystemColor.gradient02_alt_step0.color, position: 0.12),
        .init(color: LegacyDesignSystemColor.gradient02_alt_step1.color, position: 0.47),
        .init(color: LegacyDesignSystemColor.gradient02_alt_step2.color, position: 1.0),
      ],
      angle: .figmaDegrees(135)
    )
  }
  public static var gradient03: BraveGradient {
    .init(
      stops: [
        .init(color: LegacyDesignSystemColor.gradient03_step0.color, position: 0),
        .init(color: LegacyDesignSystemColor.gradient03_step1.color, position: 0.985),
      ],
      angle: .figmaDegrees(306)
    )
  }
  public static var gradient05: BraveGradient {
    .init(
      stops: [
        .init(color: LegacyDesignSystemColor.gradient05_step0.color, position: 0),
        .init(color: LegacyDesignSystemColor.gradient05_step1.color, position: 1),
      ],
      angle: .figmaDegrees(126)
    )
  }
}

extension BraveGradientView {
  public static var gradient01: BraveGradientView {
    .init { traitCollection in
      if traitCollection.userInterfaceStyle == .dark {
        return .darkGradient01
      }
      return .lightGradient01
    }
  }
  public static var gradient02: BraveGradientView {
    .init { traitCollection in
      if traitCollection.userInterfaceStyle == .dark {
        return .darkGradient02
      }
      return .lightGradient02
    }
  }
  public static var alternateGradient02: BraveGradientView {
    .init { traitCollection in
      if traitCollection.userInterfaceStyle == .dark {
        return .darkAlternateGradient02
      }
      return .lightAlternateGradient02
    }
  }
  public static var gradient03: BraveGradientView {
    .init(gradient: .gradient03)
  }
}
