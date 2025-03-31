// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct AIChatGenericErrorView: View {
  var title: String
  var onRetryRequest: () -> Void

  var body: some View {
    HStack(alignment: .top, spacing: 16.0) {
      Image(braveSystemName: "leo.warning.circle-filled")
        .foregroundStyle(Color(braveSystemName: .systemfeedbackErrorIcon))

      VStack(alignment: .leading, spacing: 16.0) {
        Text(title)
          .font(.callout)
          .foregroundColor(Color(braveSystemName: .textPrimary))

        Button(
          action: {
            onRetryRequest()
          },
          label: {
            Text(Strings.AIChat.retryActionTitle)
              .font(.headline)
              .foregroundColor(Color(braveSystemName: .schemesOnPrimary))
              .padding()
              .foregroundStyle(.white)
              .background(Color(braveSystemName: .buttonBackground), in: .capsule)
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
struct AIChatGenericErrorView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatGenericErrorView(title: Strings.AIChat.invalidApiKeyErrorViewTitle) {}
      .previewColorSchemes()
      .previewLayout(.sizeThatFits)
  }
}
#endif
