// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct AIChatMessageHeaderView: View {

  let isUser: Bool
  let isEdited: Bool

  var body: some View {
    HStack {
      if isUser {
        Image(braveSystemName: "leo.user.circle")
          .renderingMode(.template)
          .foregroundStyle(Color(braveSystemName: .iconDefault))
          .padding(8.0)
          .background(Color(braveSystemName: .containerHighlight), in: Circle())

        Text(Strings.AIChat.youMessageTitle)
          .font(.body.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      } else {
        AIChatProductIcon(containerShape: Circle(), padding: 6.0)
          .font(.callout)

        Text(Strings.AIChat.leoAssistantNameTitle)
          .font(.body.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      }

      if isEdited {
        Spacer()

        AIChatEditedBadgeView()
      }
    }
  }
}
