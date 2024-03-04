// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Favicon
import SwiftUI

struct AIChatPageContextInfoView: View {
  var url: URL?
  var pageTitle: String

  var body: some View {
    VStack(alignment: .leading) {
      Text(Strings.AIChat.leoPageContextInfoDescriptionTitle)
        .font(.caption)
        .foregroundStyle(Color(braveSystemName: .textTertiary))
        .fixedSize(horizontal: false, vertical: true)
        .frame(maxWidth: .infinity, alignment: .leading)

      Color(braveSystemName: .dividerSubtle)
        .frame(height: 1.0)

      HStack {
        FaviconImage(
          url: url?.absoluteString,
          isPrivateBrowsing: false
        )

        Text(pageTitle)
          .font(.caption)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
      }
    }
  }
}

#if DEBUG
struct AIChatPageContextInfoView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatPageContextInfoView(
      url: nil,
      pageTitle:
        "Sonos Era 300 and Era 100...'s Editors’Choice Awards: The Best AIs and Services for 2023"
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
