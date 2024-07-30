// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct BraveVPNPremiumUpsellView: View {

  var body: some View {
    VStack(spacing: 0) {
      BraveVPNUpsellTopicView(topicType: .modelType)
        .padding()

      Color(braveSystemName: .primitivePrimary25)
        .frame(height: 1.0)

      BraveVPNUpsellTopicView(topicType: .creativity)
        .padding()

      Color(braveSystemName: .primitivePrimary25)
        .frame(height: 1.0)

      BraveVPNUpsellTopicView(topicType: .accuracy)
        .padding()

      Color(braveSystemName: .primitivePrimary25)
        .frame(height: 1.0)

      BraveVPNUpsellTopicView(topicType: .chatLength)
        .padding()
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .overlay(
      ContainerRelativeShape()
        .strokeBorder(
          Color(braveSystemName: .primitivePrimary25),
          lineWidth: 1.0
        )
    )
    .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
  }
}

private struct BraveVPNUpsellTopicView: View {

  enum UpsellTopicType {
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
        return "Explore different AI models"
      case .creativity:
        return "Unlock your creativity"
      case .accuracy:
        return "Stay on topic"
      case .chatLength:
        return "Chat for longer"
      }
    }

    var subTitle: String {
      switch self {
      case .modelType:
        return "Priority access to powerful models with different skills."
      case .creativity:
        return "Access models better suited for creative tasks and content generation."
      case .accuracy:
        return "Get more accurate answers for more nuanced conversations."
      case .chatLength:
        return "Get higher rate limits for longer conversations."
      }
    }
  }
  
  let topicType: UpsellTopicType

  var body: some View {
    HStack {
      Image(braveSystemName: topicType.icon)
        .padding(8.0)
        .background(Color(braveSystemName: .primitivePrimary80))
        .foregroundColor(
          Color(braveSystemName: .primitivePrimary20)
        )
        .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))

      VStack(alignment: .leading, spacing: 6.0) {
        Text(topicType.title)
          .font(.headline)
          .foregroundStyle(Color.white)

        Text(topicType.subTitle)
          .font(.footnote)
          .foregroundStyle(
            Color(braveSystemName: .primitivePrimary80)
          )
      }
      .lineLimit(2)
      .truncationMode(.tail)
      .frame(maxWidth: .infinity, alignment: .leading)
      .fixedSize(horizontal: false, vertical: true)
    }
  }
}

#if DEBUG
#Preview("VPNUpsellView") {
  BraveVPNPremiumUpsellView()
    .padding()
    .background(
      Color(braveSystemName: .primitivePrimary10)
    )
}

#Preview("UpsellTopicTypeView") {
  BraveVPNUpsellTopicView(topicType: .creativity)
    .padding()
    .background(
      Color(braveSystemName: .primitivePrimary10)
    )
}
#endif
