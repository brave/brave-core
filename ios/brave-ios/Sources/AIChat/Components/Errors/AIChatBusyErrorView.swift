// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct AIChatBusyErrorView: View {
  var retryAction: () -> Void

  var body: some View {
    HStack(alignment: .top, spacing: 0.0) {
      Image(braveSystemName: "leo.warning.triangle-filled")
        .foregroundStyle(Color(braveSystemName: .systemfeedbackWarningIcon))
        .padding([.bottom, .trailing])

      VStack(alignment: .leading, spacing: 0.0) {
        Text(Strings.AIChat.busyErrorDescription)
          .font(.callout)
          .foregroundColor(Color(braveSystemName: .systemfeedbackWarningText))
          .padding(.bottom)

        Button(
          action: {
            retryAction()
          },
          label: {
            Text(Strings.AIChat.retryActionTitle)
              .font(.body.weight(.semibold))
              .padding()
              .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
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
    .background(Color(braveSystemName: .systemfeedbackWarningBackground))
    .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
  }
}

#if DEBUG
struct AIChatBusyErrorView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatBusyErrorView {
      print("Retry")
    }
    .previewColorSchemes()
    .previewLayout(.sizeThatFits)
  }
}
#endif
