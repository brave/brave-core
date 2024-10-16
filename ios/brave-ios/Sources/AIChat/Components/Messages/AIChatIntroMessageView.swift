// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

struct AIChatIntroMessageView: View {
  @State
  private var shouldShowInformationPopover = false

  var model: AiChat.Model

  private var modelDescription: String {
    guard let modelKey = AIChatModelKey(rawValue: model.key) else {
      return model.displayName
    }

    switch modelKey {
    case .chatBasic:
      return Strings.AIChat.introMessageLlamaModelDescription

    case .chatExpanded:
      return Strings.AIChat.introMessageMixtralModelDescription

    case .chatClaudeHaiku:
      return Strings.AIChat.introMessageClaudeHaikuModelDescription

    case .chatClaudeSonnet:
      return Strings.AIChat.introMessageClaudeSonnetModelDescription
    }
  }

  private var introMessage: String {
    guard let modelKey = AIChatModelKey(rawValue: model.key) else {
      return String(format: Strings.AIChat.introMessageGenericMessageDescription, model.displayName)
    }

    switch modelKey {
    case .chatBasic:
      return Strings.AIChat.introMessageLlamaMessageDescription

    case .chatExpanded:
      return Strings.AIChat.introMessageMixtralMessageDescription

    case .chatClaudeHaiku:
      return Strings.AIChat.introMessageClaudeHaikuMessageDescription

    case .chatClaudeSonnet:
      return Strings.AIChat.introMessageClaudeSonnetMessageDescription
    }
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      AIChatProductIcon(containerShape: Circle(), padding: 9.0)
        .font(.body)
        .padding(.trailing, 16.0)
        .padding(.bottom, 8.0)

      Text(Strings.AIChat.introMessageTitle)
        .font(.headline)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)

      HStack(alignment: .firstTextBaseline) {
        Text(modelDescription)
          .font(.footnote)
          .foregroundStyle(Color(braveSystemName: .textTertiary))
          .multilineTextAlignment(.leading)
          .fixedSize(horizontal: false, vertical: true)
          .padding(.bottom)

        Button(
          action: {
            shouldShowInformationPopover = true
          },
          label: {
            Label {
              Text(Strings.AIChat.leoPageContextInfoDescriptionTitle)
            } icon: {
              Image(braveSystemName: "leo.info.outline")
                .foregroundStyle(Color(braveSystemName: .iconDefault))
                .font(.footnote)
            }
            .labelStyle(.iconOnly)
          }
        )
        .bravePopover(
          isPresented: $shouldShowInformationPopover,
          arrowDirection: .forcedDirection(.up)
        ) {
          PopoverWrapperView(
            backgroundColor: UIColor(braveSystemName: .containerBackground)
          ) {
            VStack {
              Text(introMessage)
                .font(.footnote)
                .foregroundStyle(Color(braveSystemName: .textPrimary))
                .fixedSize(horizontal: false, vertical: true)
                .frame(maxWidth: .infinity, alignment: .leading)

              Text(
                LocalizedStringKey(
                  String(
                    format: "[%@](%@)",
                    Strings.learnMore,
                    URL.Brave.braveLeoModelCategorySupport.absoluteString
                  )
                )
              )
              .underline()
              .font(.footnote)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
              .fixedSize(horizontal: false, vertical: true)
              .frame(maxWidth: .infinity, alignment: .leading)
              .tint(Color(braveSystemName: .textInteractive))
            }
            .padding()
            .frame(maxWidth: 260.0)
          }
        }
      }
      .frame(maxWidth: .infinity, alignment: .leading)
    }
  }
}

#if DEBUG
struct AIChatIntroMessageView_Previews: PreviewProvider {
  static var previews: some View {
    AIChatIntroMessageView(
      model:
        .init(
          options: .init(
            leoModelOptions: .init(
              name: "Mixtral-8x7b",
              displayMaker: "Powerful, fast and adaptive",
              engineType: .llamaRemote,
              category: .chat,
              access: .basicAndPremium,
              maxAssociatedContentLength: 9000,
              longConversationWarningCharacterLimit: 20000
            )
          ),
          key: "mixtral_8x7b",
          displayName: "Mixtral 8x7b"
        )
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
