// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Strings
import SwiftUI

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
        Button {
          UIPasteboard.general.setSecureString(text)
        } label: {
          Text(
            "\(Strings.Wallet.copyToPasteboard) \(Image(braveSystemName: "leo.copy.plain-text"))"
          )
          .font(.subheadline)
          .foregroundColor(Color(braveSystemName: .textInteractive))
        }
      }
    }
    .padding(20)
    .background(
      Color(braveSystemName: .pageBackground).clipShape(
        RoundedRectangle(cornerRadius: 10, style: .continuous)
      )
    )
  }
}
