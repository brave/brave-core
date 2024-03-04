// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Favicon
import SwiftUI

struct AIChatPageInfoBanner: View {
  var url: URL?
  var pageTitle: String

  var body: some View {
    HStack {
      FaviconImage(
        url: url?.absoluteString,
        isPrivateBrowsing: false
      )

      Text(pageTitle)
        .font(.caption)
        .lineLimit(2)
        .truncationMode(.tail)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .padding()
    .overlay(
      ContainerRelativeShape()
        .strokeBorder(Color(braveSystemName: .dividerSubtle), lineWidth: 1.0)
    )
    .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
  }
}

#if DEBUG
struct AIChatPageInfoBanner_Preview: PreviewProvider {
  static var previews: some View {
    AIChatPageInfoBanner(
      url: nil,
      pageTitle:
        "Sonos Era 300 and Era 100...'s Editors’Choice Awards: The Best AIs and Services for 2023"
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
