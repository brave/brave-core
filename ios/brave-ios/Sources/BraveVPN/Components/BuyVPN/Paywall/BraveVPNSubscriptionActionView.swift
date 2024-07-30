// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

enum SubscriptionActionType {
  case refreshCredentials, redeemPromoCode

  var mainTitle: String {
    switch self {
    case .refreshCredentials:
      return "Already purchased on brave.com?"
    case .redeemPromoCode:
      return "Have a promo code?"
    }
  }

  var actionTitle: String {
    switch self {
    case .refreshCredentials:
      return "Refresh your credentials"
    case .redeemPromoCode:
      return "Redeem Promo Code"
    }
  }
}

struct BraveVPNSubscriptionActionView: View {
  @Binding var shouldRefreshCredentials: Bool
  
  @Binding var shouldRedeedPromoCode: Bool

  var body: some View {
    VStack(spacing: 16) {
      BraveVPNTitleActionsView(
        actionType: .refreshCredentials,
        shouldRefreshCredentials: $shouldRefreshCredentials
      )
      
      BraveVPNTitleActionsView(
        actionType: .redeemPromoCode,
        shouldRefreshCredentials: $shouldRedeedPromoCode
      )
    }
    .padding(.vertical)
  }
}

struct BraveVPNTitleActionsView: View {
  let actionType: SubscriptionActionType
  
  @Binding var shouldRefreshCredentials: Bool

  var body: some View {
    VStack(spacing: 8) {
      Text(actionType.mainTitle)
        .font(.callout.weight(.semibold))
        .foregroundColor(Color(braveSystemName: .primitivePrimary90))
      
      Button(
        action: {
          shouldRefreshCredentials = true
        },
        label: {
          HStack {
            Text(actionType.actionTitle)
              .font(.subheadline.weight(.semibold))
              .foregroundColor(Color(.white))
              .padding()
          }
          .frame(maxWidth: .infinity)
          .contentShape(ContainerRelativeShape())
        }
      )
      .overlay(
        ContainerRelativeShape()
          .strokeBorder(
            Color(braveSystemName: .dividerInteractive),
            lineWidth: 1.0
          )
      )
      .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
      .padding([.horizontal], 16.0)
    }
  }
}

#if DEBUG
struct BraveVPNRefreshCredentialsView_Previews: PreviewProvider {
  static var previews: some View {
    @State var shouldDoAction: Bool = false

    BraveVPNSubscriptionActionView(
      shouldRefreshCredentials: $shouldDoAction,
      shouldRedeedPromoCode: $shouldDoAction
    )
    .background(
      Color(braveSystemName: .primitivePrimary10)
    )
  }
}
#endif
