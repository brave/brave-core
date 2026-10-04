// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct AddressView<Content: View>: View {
  var address: String
  var content: () -> Content

  init(
    address: String,
    @ViewBuilder content: @escaping () -> Content
  ) {
    self.address = address
    self.content = content
  }

  var body: some View {
    content()
      .contextMenu {
        if !address.isEmpty {
          Text(address.zwspOutput)
          Button {
            UIPasteboard.general.string = address
          } label: {
            Label(
              Strings.Wallet.copyAddressButtonTitle,
              braveSystemImage: "leo.copy.plain-text"
            )
          }
        }
      }
  }
}
