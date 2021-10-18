/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import struct Shared.Strings

struct BuySendSwapView: View {
  var action: (BuySendSwapDestination) -> Void
  
  var body: some View {
    VStack(alignment: .leading, spacing: 16) {
      ForEach(BuySendSwapDestination.allCases, id: \.self) { action in
        Button(action: { self.action(action) }) {
          VStack(alignment: .leading, spacing: 3) {
            Text(action.localizedTitle)
              .foregroundColor(Color(.bravePrimary))
              .font(.headline)
              .multilineTextAlignment(.leading)
            Text(action.localizedDescription)
              .foregroundColor(Color(.braveLabel))
              .font(.footnote)
              .multilineTextAlignment(.leading)
          }
          .padding([.leading, .trailing], 20)
        }
        if action != BuySendSwapDestination.allCases.last {
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
