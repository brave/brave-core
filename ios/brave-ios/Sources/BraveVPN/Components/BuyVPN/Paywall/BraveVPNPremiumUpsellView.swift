// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct BraveVPNPremiumUpsellView: View {

  var body: some View {
    VStack(spacing: 8) {
      ForEach(BraveVPNUpsellTopicView.UpsellTopicType.allCases) { type in
        BraveVPNUpsellTopicView(topicType: type)
      }
    }
    .frame(maxWidth: .infinity, alignment: .leading)
  }
}

private struct BraveVPNUpsellTopicView: View {

  enum UpsellTopicType: CaseIterable, Identifiable {
    case privacy, location, server, speed, device, logs, audition

    var icon: String {
      return "leo.shield.done"
    }

    var id: String {
      title
    }

    var title: String {
      switch self {
      case .privacy:
        return Strings.VPN.infoCheckPrivacy
      case .location:
        return Strings.VPN.infoCheckLocation
      case .server:
        return Strings.VPN.infoCheckServers
      case .speed:
        return Strings.VPN.infoCheckConnectionSpeed
      case .device:
        return Strings.VPN.infoCheckLimitDevice
      case .logs:
        return Strings.VPN.infoCheckNoLogs
      case .audition:
        return Strings.VPN.infoCheckAudition
      }
    }
  }

  let topicType: UpsellTopicType

  var body: some View {
    HStack(spacing: 10) {
      Image(braveSystemName: topicType.icon)
        .foregroundStyle(
          LinearGradient(
            gradient:
              Gradient(colors: [
                Color(UIColor(rgb: 0xEC1349)),
                Color(UIColor(rgb: 0xD41173)),
              ]),
            startPoint: .init(x: 0.0, y: 0.0),
            endPoint: .init(x: 0.0, y: 1.0)
          )
        )
      Text(topicType.title)
        .font(.subheadline)
        .foregroundStyle(Color.white)
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
    .padding([.horizontal, .top], 24)
    .padding(.bottom, 16)
    .background(
      Color(braveSystemName: .primitivePrimary10)
    )
}

#Preview("UpsellTopicTypeView") {
  BraveVPNUpsellTopicView(topicType: .speed)
    .padding([.horizontal, .top], 24)
    .padding(.bottom, 16)
    .background(
      Color(braveSystemName: .primitivePrimary10)
    )
}
#endif
