// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveStrings
import DesignSystem
import Preferences
import SwiftUI

struct AIChatFeedbackPrivacyWarningView: View {
  @ObservedObject
  private var showFeedbackPrivacyWarning = Preferences.AIChat.showFeedbackPrivacyWarning

  var onCancel: () -> Void
  var onSend: () -> Void
  var openURL: (URL) -> Void

  var body: some View {
    VStack(spacing: 16.0) {
      Text(Strings.AIChat.rateAnswerFeedbackPrivacyWarningTitle)
        .font(.headline)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(Color(braveSystemName: .textPrimary))

      Text(
        LocalizedStringKey(
          String(
            format: Strings.AIChat.rateAnswerFeedbackPrivacyWarningMessage,
            URL.Brave.braveLeoPrivacyFeedbackLearnMoreLinkUrl.absoluteString
          )
        )
      )
      .font(.body)
      .frame(maxWidth: .infinity, alignment: .leading)
      .multilineTextAlignment(.leading)
      .fixedSize(horizontal: false, vertical: true)
      .foregroundStyle(Color(braveSystemName: .textSecondary))
      .environment(
        \.openURL,
        OpenURLAction { url in
          openURL(url)
          return .handled
        }
      )

      Button {
        showFeedbackPrivacyWarning.value.toggle()
      } label: {
        HStack {
          Image(
            braveSystemName: showFeedbackPrivacyWarning.value
              ? "leo.checkbox.unchecked" : "leo.checkbox.checked"
          )
          .renderingMode(.template)
          .foregroundColor(
            Color(
              braveSystemName: showFeedbackPrivacyWarning.value ? .iconDefault : .iconInteractive
            )
          )

          Text(Strings.AIChat.dontShowAgainTitle)
            .font(.subheadline)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .fixedSize(horizontal: false, vertical: true)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
      }

      HStack(spacing: 12) {
        Button {
          onCancel()
        } label: {
          Text(Strings.cancelButtonTitle)
            .font(.subheadline.weight(.semibold))
            .foregroundStyle(Color(braveSystemName: .textSecondary))
            .frame(maxWidth: .infinity)
            .padding(.vertical, 12)
        }
        .clipShape(RoundedRectangle(cornerRadius: 8))

        Button {
          onSend()
        } label: {
          Text(Strings.sendButtonTitle)
            .font(.subheadline.weight(.semibold))
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
      }
    }
    .padding(.top, 32.0)
    .padding([.leading, .trailing, .bottom], 24.0)
  }
}
