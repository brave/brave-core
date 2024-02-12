// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Strings
import SwiftUI
import DesignSystem

struct SensitiveTextView: View {
  
  var text: String
  var isCopyEnabled: Bool
  @Binding var isShowingText: Bool
  
  init(
    text: String,
    isCopyEnabled: Bool = true,
    isShowingText: Binding<Bool>
  ) {
    self.text = text
    self.isCopyEnabled = isCopyEnabled
    self._isShowingText = isShowingText
  }
  
  var body: some View {
    VStack(spacing: 32) {
      Text(text)
        .blur(radius: isShowingText ? 0 : 5)
        .accessibilityHidden(!isShowingText)
        .noCapture()
      if isCopyEnabled {
        Button(action: {
          UIPasteboard.general.setSecureString(text)
        }) {
          Text("\(Strings.Wallet.copyToPasteboard) \(Image(braveSystemName: "leo.copy.plain-text"))")
            .font(.subheadline)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
    .padding(20)
    .background(
      Color(.secondaryBraveBackground).clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
    )
  }
}
