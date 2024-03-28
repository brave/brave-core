// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
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
      .frame(maxWidth: .infinity, alignment: .leading)

      rightAccessoryView
        .padding(.horizontal, 16.0)
        .padding(.vertical, 8.0)
    }
    .padding(.vertical)
  }
}

enum AIChatMenuOptionTypes {
  case newChat
  case goPremium
  case managePremium
  case advancedSettings

  var title: String {
    switch self {
    case .newChat:
      return Strings.AIChat.quickMenuNewChatActionTitle
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
    case .newChat:
      return "leo.erase"
    case .goPremium, .managePremium:
      return "leo.lock.open"
    case .advancedSettings:
      return "leo.settings"
    }
  }
}

struct AIChatMenuView: View {
  var premiumStatus: AiChat.PremiumStatus
  var currentModel: AiChat.Model
  var modelOptions: [AiChat.Model]
  var onModelChanged: (String) -> Void
  var onOptionSelected: (AIChatMenuOptionTypes) -> Void

  @Environment(\.dismiss)
  private var dismiss

  @State
  private var appStoreConnectionErrorPresented = false

  var body: some View {
    LazyVStack(spacing: 0.0) {
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

      ForEach(modelOptions, id: \.key) { model in
        Button(
          action: {
            onModelChanged(model.key)
            dismiss()
          },
          label: {
            AIChatMenuItemView(
              title: model.displayName,
              subtitle: model.displayMaker,
              isSelected: model.key == currentModel.key
            ) {
              if model.access == .basicAndPremium {
                Text(
                  premiumStatus == .active || premiumStatus == .activeDisconnected
                    ? Strings.AIChat.unlimitedModelStatusTitle.uppercased()
                    : Strings.AIChat.limitedModelStatusTitle.uppercased()
                )
                .font(.caption2)
                .foregroundStyle(Color(braveSystemName: .blue50))
                .padding(.horizontal, 4.0)
                .padding(.vertical, 2.0)
                .background(
                  RoundedRectangle(cornerRadius: 4.0, style: .continuous)
                    .strokeBorder(Color(braveSystemName: .blue50), lineWidth: 1.0)
                )
              } else {
                Image(braveSystemName: "leo.lock.plain")
                  .foregroundStyle(Color(braveSystemName: .iconDefault))
                  .opacity(model.access == .premium ? 1.0 : 0.0)
              }
            }
          }
        )

        if let lastModel = modelOptions.last, model.key != lastModel.key {
          Color(braveSystemName: .dividerSubtle)
            .frame(height: 1.0)
        }
      }

      Color(braveSystemName: .dividerSubtle)
        .frame(height: 8.0)

      menuActionItems(for: .newChat)

      Color(braveSystemName: .dividerSubtle)
        .frame(height: 1.0)

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

  func menuActionItems(for menuOption: AIChatMenuOptionTypes) -> some View {
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
          key: "mixtral_8x7b",
          name: "Mixtral-8x7b",
          displayName: "Mixtral 8x7b",
          displayMaker: "Powerful, fast and adaptive",
          engineType: .llamaRemote,
          category: .chat,
          access: .basicAndPremium,
          maxPageContentLength: 9000,
          longConversationWarningCharacterLimit: 20000
        ),
      modelOptions: [
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
        ),
        .init(
          key: "claude_instant",
          name: "Claude-Instant",
          displayName: "Claude Instant",
          displayMaker: "Strength in creative tasks",
          engineType: .llamaRemote,
          category: .chat,
          access: .basicAndPremium,
          maxPageContentLength: 9000,
          longConversationWarningCharacterLimit: 20000
        ),
        .init(
          key: "llaba_2x13b",
          name: "Llama-2x13b",
          displayName: "Llama2 13b",
          displayMaker: "General purpose chat",
          engineType: .llamaRemote,
          category: .chat,
          access: .basic,
          maxPageContentLength: 9000,
          longConversationWarningCharacterLimit: 20000
        ),
        .init(
          key: "llaba_2x70b",
          name: "Llama-2x70b",
          displayName: "Llama2 70b",
          displayMaker: "Advanced and accurate chat",
          engineType: .llamaRemote,
          category: .chat,
          access: .premium,
          maxPageContentLength: 9000,
          longConversationWarningCharacterLimit: 20000
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
