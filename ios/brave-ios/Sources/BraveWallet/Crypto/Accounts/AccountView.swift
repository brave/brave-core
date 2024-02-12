/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI

/// Displays basic info around a single account
struct AccountView: View {
  /// The full address, which will be truncated in the middle
  var address: String
  /// The account name describing what the account is for
  var name: String

  @ScaledMetric private var avatarSize = 40.0
  private let maxAvatarSize: CGFloat = 80.0

  var body: some View {
    HStack {
      Group {
        Blockie(address: address)
          .frame(width: min(avatarSize, maxAvatarSize), height: min(avatarSize, maxAvatarSize))
      }
      VStack(alignment: .leading, spacing: 2) {
        Text(name)
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.bravePrimary))
        Text(address.truncatedAddress)
          .font(.footnote)
          .foregroundColor(Color(.braveLabel))
      }
      .font(.caption)
      Spacer()
    }
    .padding(.vertical, 6)
    .accessibilityElement()
    .accessibilityLabel("\(name), \(address.truncatedAddress)")
  }
}

#if DEBUG
struct AccountView_Previews: PreviewProvider {
  static var previews: some View {
    AccountView(
      address: "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      name: "Account 1"
    )
    .previewLayout(.sizeThatFits)
    .previewSizeCategories()
  }
}
#endif
