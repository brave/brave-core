/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import Strings

struct BackupNotifyView: View {
  var action: () -> Void
  var onDismiss: () -> Void

  private var backgroundShape: some InsettableShape {
    RoundedRectangle(cornerRadius: 8, style: .continuous)
  }

  var body: some View {
    let closeImage = Image(systemName: "xmark")
    ZStack(alignment: .topTrailing) {
      Button(action: action) {
        HStack {
          Text(Strings.Wallet.backupWalletWarningMessage)
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(.braveLabel))
            .fixedSize(horizontal: false, vertical: true)
            .frame(maxWidth: .infinity, alignment: .leading)
          closeImage.hidden()
        }
      }
      .padding(12)
      Button(action: onDismiss) {
        closeImage
          .foregroundColor(Color(.braveLabel))
          .padding(12)  // To make the hit-area a bit bigger
      }
    }
    .frame(maxWidth: .infinity)
    .background(Color(.braveErrorBackground))
    .clipShape(backgroundShape)
  }
}

#if DEBUG
struct BackupNotifyView_Previews: PreviewProvider {
  static var previews: some View {
    BackupNotifyView(action: {}, onDismiss: {})
      .padding()
      .background(Color(.braveGroupedBackground))
      .previewLayout(.sizeThatFits)
      .previewColorSchemes()
  }
}
#endif
