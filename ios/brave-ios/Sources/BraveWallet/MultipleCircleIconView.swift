// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct ContainerShape: InsettableShape {
  enum Shape {
    case circle, rectangle
  }
  let shape: Shape
  var inset: CGFloat = 0

  func inset(by amount: CGFloat) -> some InsettableShape {
    ContainerShape(shape: shape, inset: amount)
  }

  func path(in rect: CGRect) -> Path {
    let rect = rect.insetBy(dx: inset, dy: inset)
    switch shape {
    case .circle:
      return Circle().path(in: rect)
    case .rectangle:
      return RoundedRectangle(cornerRadius: 4).path(in: rect)
    }
  }
}

struct MultipleCircleIconView<IconView: View, Model>: View {
  let models: [Model]
  var shape: ContainerShape.Shape = .circle
  var maxIcons = 3
  @ScaledMetric var iconSize = 16.0
  var maxIconSize: CGFloat = 32
  @ScaledMetric var iconDotSize = 2.0

  @ViewBuilder var iconView: (Model) -> IconView

  var body: some View {
    HStack(spacing: -(min(iconSize, maxIconSize) / 2)) {
      let numberOfIcons = min(maxIcons, models.count)
      ForEach(0..<numberOfIcons, id: \.self) { index in
        buildShapeBorder {
          iconView(models[index])
            .frame(width: min(iconSize, maxIconSize) - 2, height: min(iconSize, maxIconSize) - 2)
            .clipShape(ContainerRelativeShape())
        }
        .zIndex(Double(numberOfIcons - index))
      }
      if models.count > maxIcons {
        buildShapeBorder {
          Group {
            if shape == .circle {
              Circle()
            } else {
              RoundedRectangle(cornerRadius: 4)
            }
          }
          .foregroundColor(Color(.braveBlurpleTint))
          .frame(width: min(iconSize, maxIconSize), height: min(iconSize, maxIconSize))
          .overlay(
            HStack(spacing: 1) {
              ContainerRelativeShape()
                .frame(width: iconDotSize, height: iconDotSize)
              ContainerRelativeShape()
                .frame(width: iconDotSize, height: iconDotSize)
              ContainerRelativeShape()
                .frame(width: iconDotSize, height: iconDotSize)
            }
            .foregroundColor(.white)
            .containerShape(ContainerShape(shape: shape))
          )
        }
      }
    }
  }

  /// Creates the ContainerRelativeShape with `content` overlayed, inset to show a border.
  private func buildShapeBorder(content: () -> some View) -> some View {
    Color(.secondaryBraveGroupedBackground)
      .frame(width: min(iconSize, maxIconSize), height: min(iconSize, maxIconSize))
      .clipShape(ContainerRelativeShape())
      .overlay {
        content()
          .frame(width: min(iconSize, maxIconSize) - 2, height: min(iconSize, maxIconSize) - 2)
          .clipShape(ContainerRelativeShape())
      }
      .containerShape(ContainerShape(shape: shape))
  }
}
