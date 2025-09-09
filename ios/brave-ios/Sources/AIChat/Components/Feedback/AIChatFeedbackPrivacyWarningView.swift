// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveStrings
import BraveUI
import DesignSystem
import Preferences
import SwiftUI

struct AIChatCheckboxToggleStyle: ToggleStyle {
  func makeBody(configuration: Configuration) -> some View {
    Button {
      configuration.isOn.toggle()
    } label: {
      HStack {
        Image(
          braveSystemName: configuration.isOn ? "leo.checkbox.checked" : "leo.checkbox.unchecked"
        )
        .foregroundStyle(
          Color(
            braveSystemName: configuration.isOn ? .iconInteractive : .iconDefault
          )
        )

        configuration.label
      }
      .contentShape(Rectangle())
    }
  }
}

struct AIChatFeedbackPrivacyWarningView: View {
  @State
  private var viewHeight: CGFloat?

  @State
  private var dontShowAgain = false

  var onCancel: () -> Void
  var onSend: () -> Void
  var openURL: (URL) -> Void

  var body: some View {
    ScrollView {
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

        Toggle(isOn: $dontShowAgain) {
          Text(Strings.AIChat.dontShowAgainTitle)
            .font(.subheadline)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .fixedSize(horizontal: false, vertical: true)
        }
        .toggleStyle(AIChatCheckboxToggleStyle())
        .frame(maxWidth: .infinity, alignment: .leading)

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
            Preferences.AIChat.showFeedbackPrivacyWarning.value = !dontShowAgain
            onSend()
          } label: {
            Text(Strings.sendButtonTitle)
              .font(.subheadline.weight(.semibold))
              .frame(maxWidth: .infinity)
          }
          .buttonStyle(BraveFilledButtonStyle(size: .large))
        }
      }
      .padding(24.0)
      .onGeometryChange(
        for: CGSize.self,
        of: { $0.size },
        action: {
          viewHeight = $0.height
        }
      )
    }
    .scrollBounceBehavior(.basedOnSize)
    .frame(maxHeight: viewHeight)
  }
}
