// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore

struct MultipleCircleIconView<IconView: View, Model>: View {
  let models: [Model]
  let maxIcons = 3
  @ScaledMetric var iconSize = 16.0
  var maxIconSize: CGFloat = 32
  @ScaledMetric var iconDotSize = 2.0

  @ViewBuilder var iconView: (Model) -> IconView

  var body: some View {
    HStack(spacing: -(min(iconSize, maxIconSize) / 2)) {
      let numberOfIcons = min(maxIcons, models.count)
      ForEach(0..<numberOfIcons, id: \.self) { index in
        iconView(models[index])
          .frame(width: min(iconSize, maxIconSize), height: min(iconSize, maxIconSize))
          .overlay(Circle().stroke(Color(.secondaryBraveGroupedBackground), lineWidth: 1))
          .zIndex(Double(numberOfIcons - index))
      }
      if models.count > maxIcons {
        Circle()
          .foregroundColor(Color(.braveBlurpleTint))
          .frame(width: min(iconSize, maxIconSize), height: min(iconSize, maxIconSize))
          .overlay(
            HStack(spacing: 1) {
              Circle()
                .frame(width: iconDotSize, height: iconDotSize)
              Circle()
                .frame(width: iconDotSize, height: iconDotSize)
              Circle()
                .frame(width: iconDotSize, height: iconDotSize)
            }
              .foregroundColor(.white)
          )
      }
    }
  }
}
