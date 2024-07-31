// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct AIChatPremiumUpsellView: View {
  enum UpsellType {
    case premium, rateLimit

    var title: String {
      switch self {
      case .premium:
        return Strings.AIChat.paywallPremiumUpsellTitle
      case .rateLimit:
        return Strings.AIChat.paywallRateLimitTitle
      }
    }

    var subtitle: String? {
      switch self {
      case .rateLimit:
        return Strings.AIChat.paywallRateLimitSubTitle
      default:
        return nil
      }
    }

    var primaryActionTitle: String {
      switch self {
      case .premium, .rateLimit:
        return Strings.AIChat.paywallPremiumUpsellPrimaryAction
      }
    }

    var dismissActionTitle: String {
      return Strings.AIChat.paywallPremiumUpsellDismissAction
    }
  }

  var upsellType: UpsellType
  var upgradeAction: (() -> Void)?
  var dismissAction: (() -> Void)?

  var body: some View {
    VStack(spacing: 0.0) {
      PremiumUpsellTitleView(
        upsellType: upsellType,
        isPaywallPresented: false
      )
      .padding(24.0)
      PremiumUpsellDetailView(
        isPaywallPresented: false
      )
      .padding(8.0)
      PremiumUpsellActionView(
        upsellType: upsellType,
        upgradeAction: {
          upgradeAction?()
        },
        dismissAction: {
          dismissAction?()
        }
      )
    }
    .background(Color(braveSystemName: .primary10))
    .frame(maxWidth: .infinity, alignment: .leading)
    .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
  }
}

struct PremiumUpsellActionView: View {
  let upsellType: AIChatPremiumUpsellView.UpsellType
  var upgradeAction: (() -> Void)?
  var dismissAction: (() -> Void)?

  var body: some View {
    Button(
      action: {
        upgradeAction?()
      },
      label: {
        Text(upsellType.primaryActionTitle)
          .font(.subheadline.weight(.medium))
          .padding([.top, .bottom], 12.0)
          .padding([.leading, .trailing], 16.0)
          .frame(maxWidth: .infinity)
          .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
          .background(
            Color(braveSystemName: .buttonBackground),
            in: RoundedRectangle(cornerRadius: 12.0, style: .continuous)
          )
          .padding(16.0)
      }
    )
    .buttonStyle(.plain)

    Button(
      action: {
        dismissAction?()
      },
      label: {
        Text(upsellType.dismissActionTitle)
          .font(.subheadline.weight(.medium))
          .padding([.top, .bottom], 12.0)
          .padding([.leading, .trailing], 16.0)
          .frame(maxWidth: .infinity)
          .background(.clear)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
          .padding([.bottom], 12.0)
          .padding([.leading, .trailing], 16.0)
      }
    )
    .buttonStyle(.plain)
  }
}

struct PremiumUpsellTitleView: View {
  let upsellType: AIChatPremiumUpsellView.UpsellType

  let isPaywallPresented: Bool

  var foregroundTextColor: Color {
    isPaywallPresented ? Color.white : Color(braveSystemName: .textPrimary)
  }

  var body: some View {
    switch upsellType {
    case .premium:
      Text(upsellType.title)
        .font(.body.weight(.medium))
        .lineLimit(2)
        .truncationMode(.tail)
        .frame(maxWidth: .infinity, alignment: .center)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(foregroundTextColor)
    case .rateLimit:
      VStack(alignment: .leading, spacing: 8) {
        Text(upsellType.title)
          .font(.body.weight(.medium))
          .lineLimit(2)
          .truncationMode(.tail)
        if let subtitle = upsellType.subtitle {
          Text(subtitle)
            .font(.footnote)
        }
      }
      .frame(maxWidth: .infinity, alignment: .leading)
      .fixedSize(horizontal: false, vertical: true)
      .foregroundStyle(foregroundTextColor)
    }
  }
}

struct PremiumUpsellDetailView: View {
  let isPaywallPresented: Bool

  var body: some View {
    VStack(spacing: 0) {
      PremiumUpsellTopicView(
        topicType: .modelType,
        isPaywallPresented: isPaywallPresented
      )
      .padding()

      Color(braveSystemName: isPaywallPresented ? .primitivePrimary25 : .primary20)
        .frame(height: 1.0)

      PremiumUpsellTopicView(
        topicType: .creativity,
        isPaywallPresented: isPaywallPresented
      )
      .padding()

      Color(braveSystemName: isPaywallPresented ? .primitivePrimary25 : .primary20)
        .frame(height: 1.0)

      PremiumUpsellTopicView(
        topicType: .accuracy,
        isPaywallPresented: isPaywallPresented
      )
      .padding()

      Color(braveSystemName: isPaywallPresented ? .primitivePrimary25 : .primary20)
        .frame(height: 1.0)

      PremiumUpsellTopicView(
        topicType: .chatLength,
        isPaywallPresented: isPaywallPresented
      )
      .padding()
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .overlay(
      ContainerRelativeShape()
        .strokeBorder(
          Color(braveSystemName: isPaywallPresented ? .primitivePrimary25 : .primary20),
          lineWidth: 1.0
        )
    )
    .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
  }
}

private struct PremiumUpsellTopicView: View {

  let topicType: UpsellTopicType

  let isPaywallPresented: Bool

  var body: some View {
    HStack {
      Image(braveSystemName: topicType.icon)
        .padding(8.0)
        .background(Color(braveSystemName: isPaywallPresented ? .primitivePrimary80 : .primary20))
        .foregroundColor(
          Color(braveSystemName: isPaywallPresented ? .primitivePrimary20 : .primary60)
        )
        .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))

      VStack(alignment: .leading, spacing: 6.0) {
        Text(topicType.title)
          .font(
            isPaywallPresented
              ? .headline
              : .subheadline.weight(.semibold)
          )
          .foregroundStyle(
            isPaywallPresented
              ? Color.white
              : Color(braveSystemName: .textPrimary)
          )

        Text(topicType.subTitle)
          .font(.footnote)
          .foregroundStyle(
            Color(braveSystemName: isPaywallPresented ? .primitivePrimary80 : .textSecondary)
          )
      }
      .lineLimit(2)
      .truncationMode(.tail)
      .frame(maxWidth: .infinity, alignment: .leading)
      .fixedSize(horizontal: false, vertical: true)
    }
  }

  fileprivate enum UpsellTopicType {
    case modelType, creativity, accuracy, chatLength

    var icon: String {
      switch self {
      case .modelType:
        return "leo.widget.generic"
      case .creativity:
        return "leo.idea"
      case .accuracy:
        return "leo.edit.pencil"
      case .chatLength:
        return "leo.message.bubble-comments"
      }
    }

    var title: String {
      switch self {
      case .modelType:
        return Strings.AIChat.paywallUpsellModelTypeTopicTitle
      case .creativity:
        return Strings.AIChat.paywallUpsellCreativityTopicTitle
      case .accuracy:
        return Strings.AIChat.paywallUpsellAccuracyTopicTitle
      case .chatLength:
        return Strings.AIChat.paywallUpsellChatLengthTopicTitle
      }
    }

    var subTitle: String {
      switch self {
      case .modelType:
        return Strings.AIChat.paywallUpsellModelTypeTopicSubTitle
      case .creativity:
        return Strings.AIChat.paywallUpsellCreativityTopicSubTitle
      case .accuracy:
        return Strings.AIChat.paywallUpsellAccuracyTopicSubTitle
      case .chatLength:
        return Strings.AIChat.paywallUpsellChatLengthTopicSubTitle
      }
    }
  }
}

#if DEBUG
struct AIChatPremiumUpsellView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatPremiumUpsellView(upsellType: .premium)
      .previewColorSchemes()
      .previewLayout(.sizeThatFits)
  }
}
#endif
