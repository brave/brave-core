// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct RecoveryPhrasePager: View {
  var pageNumber: Int = 3
  var activePagerWidth: CGFloat = 20
  var pagerWidth: CGFloat = 14
  var pagerHeight: CGFloat = 8
  var activePagerColor: Color = Color(.braveBlurpleTint)
  var inactivePagerColor: Color = Color(.braveDisabled)

  @Binding var activeIndex: Int

  var body: some View {
    HStack(spacing: 4) {
      ForEach(0..<pageNumber, id: \.self) { index in
        if index == activeIndex {
          Rectangle()
            .frame(width: activePagerWidth, height: pagerHeight)
            .foregroundColor(activePagerColor)
            .cornerRadius(pagerHeight)
        } else {
          Rectangle()
            .frame(width: pagerWidth, height: pagerHeight)
            .foregroundColor(inactivePagerColor)
            .cornerRadius(pagerHeight)
        }
      }
    }
  }
}

#if DEBUG
struct RecoveryPhrasePager_Previews: PreviewProvider {
  static var previews: some View {
    RecoveryPhrasePager(activeIndex: .constant(0))
  }
}
#endif
