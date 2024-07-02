// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct AIChatUserMessageView: View {
  var prompt: String

  var body: some View {
    VStack(alignment: .leading) {
      HStack {
        Image(braveSystemName: "leo.user.circle")
          .renderingMode(.template)
          .foregroundStyle(Color(braveSystemName: .iconDefault))
          .padding(8.0)
          .background(Color(braveSystemName: .containerHighlight), in: Circle())

        Text("You")
          .font(.body.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      }

      Text(prompt)
        .font(.subheadline)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
    }
  }
}

#if DEBUG
struct AIChatUserMessageView_Previews: PreviewProvider {
  static var previews: some View {
    AIChatUserMessageView(
      prompt: "Does it work with Apple devices?"
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
