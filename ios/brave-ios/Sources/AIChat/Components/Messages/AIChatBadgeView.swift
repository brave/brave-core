// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct AIChatBadgeView: View {

  let text: String

  var body: some View {
    Text(text)
      .foregroundStyle(Color(braveSystemName: .neutral60))
      .font(.caption2.weight(.bold))
      .padding(4)
      .background(
        RoundedRectangle(cornerRadius: 4)
          .fill(Color(braveSystemName: .neutral20))
      )
  }
}

struct AIChatEditedBadgeView: View {
  var body: some View {
    AIChatBadgeView(
      text: Strings.AIChat.editedMessageCaption.uppercased()
    )
  }
}

#if DEBUG
struct AIChatEditedBadgeView_Preview: PreviewProvider {

  static var previews: some View {
    AIChatEditedBadgeView()
  }
}
#endif
