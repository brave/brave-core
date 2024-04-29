// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

struct AIChatIntroMessageView: View {
  var model: AiChat.Model

  private enum ModelKey: String {
    case chatBasic = "chat-basic"
    case chatExpanded = "chat-leo-expanded"
    case chatClaudeInstant = "chat-claude-instant"
  }

  private var modelDescription: String {
    guard let modelKey = ModelKey(rawValue: model.key) else {
      return model.displayName
    }

    switch modelKey {
    case .chatBasic:
      return Strings.AIChat.introMessageLlamaModelDescription

    case .chatExpanded:
      return Strings.AIChat.introMessageMixtralModelDescription

    case .chatClaudeInstant:
      return Strings.AIChat.introMessageClaudeInstantModelDescription
    }
  }

  private var introMessage: String {
    guard let modelKey = ModelKey(rawValue: model.key) else {
      return String(format: Strings.AIChat.introMessageGenericMessageDescription, model.displayName)
    }

    switch modelKey {
    case .chatBasic:
      return Strings.AIChat.introMessageLlamaMessageDescription

    case .chatExpanded:
      return Strings.AIChat.introMessageMixtralMessageDescription

    case .chatClaudeInstant:
      return Strings.AIChat.introMessageClaudeInstantMessageDescription
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

      Text(modelDescription)
        .font(.footnote)
        .foregroundStyle(Color(braveSystemName: .textTertiary))
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.bottom)

      Text(introMessage)
        .font(.subheadline)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
    }
  }
}

#if DEBUG
struct AIChatIntroMessageView_Previews: PreviewProvider {
  static var previews: some View {
    AIChatIntroMessageView(
      model:
        .init(
          key: "mixtral_8x7b",
          name: "Mixtral-8x7b",
          displayName: "Mixtral 8x7b",
          displayMaker: "Powerful, fast and adaptive",
          engineType: .llamaRemote,
          category: .chat,
          access: .basicAndPremium,
          maxPageContentLength: 9000,
          longConversationWarningCharacterLimit: 20000
        )
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
