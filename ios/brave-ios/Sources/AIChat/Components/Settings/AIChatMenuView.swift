// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStore
import SwiftUI

private struct AIChatMenuHeaderView: View {
  var icon: String
  var title: String

  var body: some View {
    HStack(spacing: 8.0) {
      Image(braveSystemName: icon)
        .foregroundStyle(Color(braveSystemName: .iconDefault))

      Text(title)
        .font(.caption2.weight(.semibold))
        .foregroundStyle(Color(braveSystemName: .textTertiary))
        .frame(maxWidth: .infinity, alignment: .leading)
    }
    .padding(.leading, 16.0)
    .padding(.vertical, 8.0)
  }
}

private struct AIChatMenuItemView<RightAccessoryView: View>: View {
  var title: String
  var subtitle: String
  var isSelected: Bool
  var rightAccessoryView: RightAccessoryView

  init(
    title: String,
    subtitle: String,
    isSelected: Bool,
    @ViewBuilder _ rightAccessoryView: () -> RightAccessoryView
  ) {
    self.title = title
    self.subtitle = subtitle
    self.isSelected = isSelected
    self.rightAccessoryView = rightAccessoryView()
  }

  var body: some View {
    HStack(spacing: 0.0) {
      Image(braveSystemName: "leo.check.normal")
        .foregroundStyle(Color(braveSystemName: .textInteractive))
        .padding(.leading, 16.0)
        .padding(.trailing, 8.0)
        .opacity(isSelected ? 1.0 : 0.0)

      VStack(alignment: .leading) {
        Text(title)
          .foregroundStyle(Color(braveSystemName: .textPrimary))

        Text(subtitle)
          .font(.footnote)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
      }
      .multilineTextAlignment(.leading)
      .frame(maxWidth: .infinity, alignment: .leading)

      rightAccessoryView
        .padding(.horizontal, 16.0)
        .padding(.vertical, 8.0)
    }
    .padding(.vertical)
  }
}

enum AIChatMenuOptionTypes {
  case goPremium
  case managePremium
  case advancedSettings

  var title: String {
    switch self {
    case .goPremium:
      return Strings.AIChat.quickMenuGoPremiumActionTitle
    case .managePremium:
      return Strings.AIChat.quickMenuManageSubscriptionActionTitle
    case .advancedSettings:
      return Strings.AIChat.quickMenuAdvancedSettingsActionTitle
    }
  }

  var imageName: String {
    switch self {
    case .goPremium, .managePremium:
      return "leo.lock.open"
    case .advancedSettings:
      return "leo.settings"
    }
  }
}

struct AIChatMenuView: View {
  var premiumStatus: AiChat.PremiumStatus
  var currentModel: AiChat.Model?
  var modelOptions: [AiChat.Model]
  var onModelChanged: (String) -> Void
  var onOptionSelected: (AIChatMenuOptionTypes) -> Void

  @Environment(\.dismiss)
  private var dismiss

  @State
  private var appStoreConnectionErrorPresented = false

  private var regularModels: [AiChat.Model] {
    modelOptions.filter({
      $0.options.tag == .leoModelOptions
    })
  }

  private var customModels: [AiChat.Model] {
    modelOptions.filter({
      $0.options.tag == .customModelOptions
    })
  }

  var body: some View {
    VStack(spacing: 0.0) {
      Text(Strings.AIChat.defaultModelLanguageSectionTitle)
        .font(.caption2.weight(.semibold))
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.horizontal, 24.0)
        .padding(.vertical)
        .background(Color(braveSystemName: .pageBackground))

      Color(braveSystemName: .dividerSubtle)
        .frame(height: 1.0)

      AIChatMenuHeaderView(
        icon: "leo.message.bubble-comments",
        title: Strings.AIChat.chatMenuSectionTitle.uppercased()
      )

      Color(braveSystemName: .dividerSubtle)
        .frame(height: 1.0)

      ForEach(regularModels, id: \.key) { model in
        viewForModel(model)
      }

      if !customModels.isEmpty {
        Color(braveSystemName: .dividerSubtle)
          .frame(height: 1.0)

        Text(Strings.AIChat.customModelsMenuSectionTitle.uppercased())
          .font(.caption2.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .frame(maxWidth: .infinity, alignment: .leading)
          .padding(.horizontal, 24.0)
          .padding(.vertical)
          .background(Color(braveSystemName: .pageBackground))

        ForEach(customModels, id: \.key) { model in
          viewForModel(model)
        }
      }

      Color(braveSystemName: .dividerSubtle)
        .frame(height: 8.0)

      // Check if leo in-app purchase is activated before or not
      if let state = BraveStoreSDK.shared.leoSubscriptionStatus?.state {
        if premiumStatus != .active && premiumStatus != .activeDisconnected {
          menuActionItems(for: .goPremium)
        } else {
          // There is prior in-app purchase
          switch state {
          case .subscribed, .inGracePeriod, .inBillingRetryPeriod:
            menuActionItems(for: .managePremium)
          case .expired, .revoked:
            menuActionItems(for: .goPremium)
          default:
            menuActionItems(for: .goPremium)
          }
        }
      } else {
        // If there is no prior in-app purchase
        // Checking premium active status
        if premiumStatus != .active && premiumStatus != .activeDisconnected {
          menuActionItems(for: .goPremium)
        } else {
          menuActionItems(for: .managePremium)
        }
      }

      Color(braveSystemName: .dividerSubtle)
        .frame(height: 1.0)

      menuActionItems(for: .advancedSettings)
    }
    .background(Color(braveSystemName: .containerBackground))
    .alert(isPresented: $appStoreConnectionErrorPresented) {
      Alert(
        title: Text(Strings.AIChat.appStoreErrorTitle),
        message: Text(Strings.AIChat.appStoreErrorSubTitle),
        dismissButton: .default(Text(Strings.OKString))
      )
    }
  }

  @ViewBuilder
  private func viewForModel(_ model: AiChat.Model) -> some View {
    Button(
      action: {
        onModelChanged(model.key)
        dismiss()
      },
      label: {
        AIChatMenuItemView(
          title: model.displayName,
          subtitle: modelPurpose(for: model),
          isSelected: model.key == currentModel?.key
        ) {
          switch model.options.tag {
          case .leoModelOptions:
            if model.options.leoModelOptions?.access == .premium && premiumStatus != .active
              && premiumStatus != .activeDisconnected
            {
              Text(
                Strings.AIChat.premiumModelStatusTitle.uppercased()
              )
              .font(.caption2)
              .foregroundStyle(Color(braveSystemName: .blue50))
              .padding(.horizontal, 4.0)
              .padding(.vertical, 2.0)
              .background(
                RoundedRectangle(cornerRadius: 4.0, style: .continuous)
                  .strokeBorder(Color(braveSystemName: .blue50), lineWidth: 1.0)
              )
            }
          case .customModelOptions, .null:
            EmptyView()
          @unknown default:
            EmptyView()
          }
        }
      }
    )

    if let lastModel = modelOptions.last, model.key != lastModel.key {
      Color(braveSystemName: .dividerSubtle)
        .frame(height: 1.0)
    }
  }

  private func modelPurpose(for model: AiChat.Model) -> String {
    guard let modelKey = AIChatModelKey(rawValue: model.key) else {
      return model.displayName
    }

    switch modelKey {
    case .chatBasic:
      return Strings.AIChat.introMessageLlamaModelPurposeDescription

    case .chatExpanded:
      return Strings.AIChat.introMessageMixtralModelPurposeDescription

    case .chatClaudeHaiku:
      return Strings.AIChat.introMessageClaudeHaikuModelPurposeDescription

    case .chatClaudeSonnet:
      return Strings.AIChat.introMessageClaudeSonnetModelPurposeDescription
    }
  }

  private func menuActionItems(for menuOption: AIChatMenuOptionTypes) -> some View {
    Button {
      if menuOption == .goPremium, !BraveStoreSDK.shared.isLeoProductsLoaded {
        appStoreConnectionErrorPresented = true
      } else {
        dismiss()
        onOptionSelected(menuOption)
      }
    } label: {
      Text(menuOption.title)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding()

      Image(braveSystemName: menuOption.imageName)
        .foregroundStyle(Color(braveSystemName: .iconDefault))
        .padding(.horizontal, 16.0)
        .padding(.vertical, 8.0)
    }
  }
}

#if DEBUG
struct AIChatMenuView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatMenuView(
      premiumStatus: .inactive,
      currentModel:
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
        ),
      modelOptions: [
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
        ),
        .init(
          options: .init(
            leoModelOptions: .init(
              name: "Claude-Instant",
              displayMaker: "Strength in creative tasks",
              engineType: .claudeRemote,
              category: .chat,
              access: .basicAndPremium,
              maxAssociatedContentLength: 9000,
              longConversationWarningCharacterLimit: 20000
            )
          ),
          key: "claude_instant",
          displayName: "Claude-Instant"
        ),
        .init(
          options: .init(
            leoModelOptions: .init(
              name: "Llama-2x13b",
              displayMaker: "General purpose chat",
              engineType: .llamaRemote,
              category: .chat,
              access: .basicAndPremium,
              maxAssociatedContentLength: 9000,
              longConversationWarningCharacterLimit: 20000
            )
          ),
          key: "llama_2x13b",
          displayName: "Llama-2 13b"
        ),
        .init(
          options: .init(
            leoModelOptions: .init(
              name: "Llama-2x70b",
              displayMaker: "Advanced and accurate chat",
              engineType: .llamaRemote,
              category: .chat,
              access: .premium,
              maxAssociatedContentLength: 9000,
              longConversationWarningCharacterLimit: 20000
            )
          ),
          key: "llama_2x70b",
          displayName: "Llama-2 70b"
        ),
      ],
      onModelChanged: {
        print("Model Changed To: \($0)")
      },
      onOptionSelected: { _ in }
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
