// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct WebUISendView: View {

  var body: some View {
    ChromeWebView(title: Strings.Wallet.send, urlString: "brave://wallet/send")
      .navigationTitle(Strings.Wallet.send)
      .navigationBarTitleDisplayMode(.inline)
  }
}
