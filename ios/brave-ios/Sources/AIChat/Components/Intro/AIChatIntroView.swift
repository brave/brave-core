// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

private struct AIChatIntroBubbleView: View {
  var title: String
  var subtitle: String
  var onSummarizePage: (() -> Void)?

  var body: some View {
    VStack(spacing: 0.0) {
      VStack(alignment: .leading, spacing: 8.0) {
        Text(title)
          .font(.callout.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textPrimary))

        Text(subtitle)
          .font(.footnote)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
      }
      .frame(maxWidth: .infinity, alignment: .leading)
      .fixedSize(horizontal: false, vertical: true)

      if let onSummarizePage = onSummarizePage {
        Button {
          onSummarizePage()
        } label: {
          Text(Strings.AIChat.summarizePageActionTitle)
            .font(.callout)
            .foregroundColor(Color(braveSystemName: .textInteractive))
            .padding(.horizontal, 12.0)
            .padding(.vertical, 8.0)
            .overlay(
              ContainerRelativeShape()
                .strokeBorder(Color(braveSystemName: .dividerInteractive), lineWidth: 1.0)
            )
            .containerShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.top, 24.0)
      }
    }
    .padding(24.0)
  }
}

struct AIChatIntroView: View {
  let onSummarizePage: (() -> Void)?

  var body: some View {
    VStack(spacing: 0.0) {
      VStack(alignment: .leading, spacing: 8.0) {
        Text(Strings.AIChat.chatIntroTitle)
          .font(.largeTitle.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textPrimary))

        Text(Strings.AIChat.chatIntroSubTitle)
          .font(.title.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      }
      .frame(maxWidth: .infinity, alignment: .leading)
      .fixedSize(horizontal: false, vertical: true)
      .padding(.horizontal, 24.0)
      .padding(.bottom, 36.0)

      AIChatIntroBubbleView(
        title: Strings.AIChat.chatIntroWebsiteHelpTitle,
        subtitle: onSummarizePage != nil
          ? Strings.AIChat.chatIntroWebsiteHelpSubtitlePageSummarize
          : Strings.AIChat.chatIntroWebsiteHelpSubtitleArticleSummarize,
        onSummarizePage: onSummarizePage
      )
      .background(Color(braveSystemName: .purple10))
      .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
      .padding([.horizontal, .bottom], 12.0)

      AIChatIntroBubbleView(
        title: Strings.AIChat.chatIntroJustTalkTitle,
        subtitle: Strings.AIChat.chatIntroJustTalkSubTitle,
        onSummarizePage: nil
      )
      .background(Color(braveSystemName: .teal10))
      .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
      .padding([.horizontal, .bottom], 12.0)
    }
  }
}

#if DEBUG
struct AIChatIntroView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatIntroView(onSummarizePage: nil)
      .previewLayout(.sizeThatFits)
  }
}
#endif
