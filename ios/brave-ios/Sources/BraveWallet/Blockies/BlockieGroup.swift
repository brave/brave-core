// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// Displays 2 blockies on top of each other representing a transactional relationship
///
/// By default, creating a `BlockieGroup` will result in a dynamically sized icon based
/// on the users size category. If you for some reason need to obtain a fixed size asset icon,
/// wrap this view in another frame of your desired size, for example:
///
///     BlockieGroup(fromAddress: "...", toAddress: "...")
///       .frame(width: 20, height: 20)
///
/// **Alignment**
///
/// You may want to set `alignVisuallyCentered` based on the usage of the `BlockieGroup`
///
/// A `BlockieGroup` will visually center the large `fromAddress` blockie creating extra
/// padding on the leading side unless `alignVisuallyCentered` is set to `false`. Set this
/// to `false` when you are displaying a `BlockieGroup` in a non-centere aligned container
/// such as when it will line up with the edge of its container. When a `BlockieGroup` appears
/// centered within a container ensure `alignVisuallyCentered` is set to `true`
struct BlockieGroup: View {
  /// The large blockie's address
  var fromAddress: String
  /// The small blockie's address
  var toAddress: String
  /// The base size of the large blockie. This value will scale with the users size category
  @ScaledMetric var size = 40.0
  /// Whether or not additional padding is added to the group to keep the main blockie
  /// visually centered
  var alignVisuallyCentered = true

  var body: some View {
    ZStack(alignment: .trailing) {
      Blockie(address: fromAddress)
        .frame(width: size, height: size)
      Blockie(address: toAddress)
        .frame(width: size * 0.6, height: size * 0.6)
        .alignmentGuide(.trailing) { d in
          d[HorizontalAlignment.center]
        }
    }
    .padding(.leading, alignVisuallyCentered ? size * 0.3 : 0)  // Visually center the main blockie
  }
}

#if DEBUG
struct BlockieGroup_Previews: PreviewProvider {
  static var previews: some View {
    ZStack {
      // Guides for visual alignment inspection
      HStack {
        Rectangle()
          .fill(Color.red)
          .frame(width: 1, height: 100)
        Spacer()
        Rectangle()
          .fill(Color.red)
          .frame(width: 1, height: 100)
        Spacer()
        Rectangle()
          .fill(Color.red)
          .frame(width: 1, height: 100)
      }
      .layoutPriority(-1)
      BlockieGroup(
        fromAddress: "0xddfabcdc4d8adc6d5beaf154f11b778f892a0740",
        toAddress: "0xbd5b489e2177f20a0779dff0faa834ca834bcd39"
      )
    }
    .padding()
    .previewLayout(.sizeThatFits)
    .previewSizeCategories()
  }
}
#endif
