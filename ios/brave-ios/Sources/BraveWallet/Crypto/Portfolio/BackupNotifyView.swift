// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Strings
import SwiftUI

struct BackupNotifyView: View {
  var action: () -> Void
  var onDismiss: () -> Void

  private var backgroundShape: some InsettableShape {
    RoundedRectangle(cornerRadius: 8, style: .continuous)
  }

  var body: some View {
    let closeImage = Image(systemName: "xmark")
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
    .frame(maxWidth: .infinity)
    .background(Color(.braveErrorBackground))
    .hoverEffect()
    .clipShape(backgroundShape)
    .overlay(alignment: .topTrailing) {
      Button(action: onDismiss) {
        closeImage
          .foregroundColor(Color(.braveLabel))
          .padding(12)  // To make the hit-area a bit bigger
      }
      .contentShape(.hoverEffect, .rect(cornerRadius: 4).inset(by: -4))
      .hoverEffect()
    }
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
