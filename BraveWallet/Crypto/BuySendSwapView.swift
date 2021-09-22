/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI

struct BuySendSwapView: View {
  enum Action: CaseIterable {
    case buy
    case send
    case swap
    
    var title: String {
      switch self {
      case .buy:
        return "Buy" // NSLocalizedString
      case .send:
        return "Send" // NSLocalizedString
      case .swap:
        return "Swap" // NSLocalizedString
      }
    }
    
    var description: String {
      switch self {
      case .buy:
        return "Buy crypto with Apple Pay, credit or debit card." // NSLocalizedString
      case .send:
        return "Send crypto or transfer from one account to another." // NSLocalizedString
      case .swap:
        return "Swap crypto assets with Brave DEX aggregator." // NSLocalizedString
      }
    }
  }
  
  var action: (Action) -> Void
  
  var body: some View {
    VStack(alignment: .leading, spacing: 16) {
      ForEach(Action.allCases, id: \.self) { action in
        Button(action: { self.action(action) }) {
          VStack(alignment: .leading, spacing: 3) {
            Text(action.title)
              .foregroundColor(Color(.bravePrimary))
              .font(.headline)
              .multilineTextAlignment(.leading)
            Text(action.description)
              .foregroundColor(Color(.braveLabel))
              .font(.footnote)
              .multilineTextAlignment(.leading)
          }
          .padding([.leading, .trailing], 20)
        }
        if action != Action.allCases.last {
          Divider()
            .padding(.leading, 20)
        }
      }
    }
    .padding(.vertical, 20)
    .background(Color(.braveBackground))
  }
}

#if DEBUG
struct BuySendSwapView_Previews: PreviewProvider {
  static var previews: some View {
    BuySendSwapView(action: { _ in })
      .previewLayout(.sizeThatFits)
//      .previewColorSchemes()
      .previewSizeCategories([.large, .accessibilityLarge])
  }
}
#endif
