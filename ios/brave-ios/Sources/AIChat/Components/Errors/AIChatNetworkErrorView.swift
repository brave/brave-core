// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct AIChatNetworkErrorView: View {
  var onRetryRequest: () -> Void

  var body: some View {
    HStack(alignment: .top, spacing: 0.0) {
      Image(braveSystemName: "leo.warning.circle-filled")
        .foregroundStyle(Color(braveSystemName: .systemfeedbackErrorIcon))
        .padding([.bottom, .trailing])

      VStack(alignment: .leading, spacing: 0.0) {
        Text(Strings.AIChat.networkErrorViewTitle)
          .font(.callout)
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .padding(.bottom)

        Button(
          action: {
            onRetryRequest()
          },
          label: {
            Text(Strings.AIChat.retryActionTitle)
              .font(.body.weight(.semibold))
              .foregroundColor(Color(braveSystemName: .schemesOnPrimary))
              .padding()
              .foregroundStyle(.white)
              .background(
                Color(braveSystemName: .buttonBackground),
                in: Capsule()
              )
          }
        )
        .buttonStyle(.plain)
      }
    }
    .padding()
    .background(Color(braveSystemName: .systemfeedbackErrorBackground))
    .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
  }
}

#if DEBUG
struct AIChatNetworkErrorView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatNetworkErrorView {}
      .previewColorSchemes()
      .previewLayout(.sizeThatFits)
  }
}
#endif
