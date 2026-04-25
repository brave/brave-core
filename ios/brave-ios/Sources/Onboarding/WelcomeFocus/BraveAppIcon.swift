// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import SwiftUI

/// A View that renders what is basically the Brave app icon
struct BraveAppIcon: View {
  struct MatchedGeometryInfo {
    var namespace: Namespace.ID
    var isSource: Bool = true
  }

  var size: CGFloat
  var matchedGeometryInfo: MatchedGeometryInfo?

  var body: some View {
    Image(sharedName: "brave.logo")
      .resizable()
      .padding((size * 0.02).rounded())
      .background(
        Color(braveSystemName: .containerBackground)
          .shadow(.drop(color: Color(braveSystemName: .elevationSecondary), radius: 4, y: 8))
          .shadow(.drop(color: Color(braveSystemName: .elevationPrimary), radius: 0, y: 1)),
        in: .rect(cornerRadius: size / 4.44, style: .continuous)
      )
      .matchedGeometryEffect(
        id: "logo",
        in: matchedGeometryInfo?.namespace ?? Namespace().wrappedValue,
        isSource: matchedGeometryInfo?.isSource ?? true
      )
      .frame(width: size, height: size)
  }
}
