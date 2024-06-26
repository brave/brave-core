// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct WebUISwapView: View {

  @State private var title: String = Strings.Wallet.swap
  @Environment(\.openURL) private var openWalletURL

  var body: some View {
    ChromeWebView(
      urlString: "brave://wallet/swap",
      title: $title,
      openURL: { url in
        openWalletURL(url)
      }
    )
    .navigationTitle(title)
    .navigationBarTitleDisplayMode(.inline)
    .background(Color(braveSystemName: .containerBackground))
    .ignoresSafeArea(.keyboard, edges: .bottom)
  }
}
