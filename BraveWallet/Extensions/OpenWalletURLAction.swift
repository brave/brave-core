// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// Provides functionality for opening a wallet URL in Brave's browser
@available(iOS, deprecated: 15.0, message: "Use `OpenURLAction.init(handler:)`")
struct OpenWalletURLAction {
  private var action: (URL) -> Void
  init(action: @escaping (URL) -> Void) {
    self.action = action
  }
  func callAsFunction(_ url: URL) {
    action(url)
  }
}

private struct OpenWalletURLActionKey: EnvironmentKey {
  static var defaultValue: OpenWalletURLAction?
}

extension EnvironmentValues {
  /// Opens a URL using the ``OpenWalletURLAction`` from the environment.
  ///
  /// Set this value to handle when wallet wants to open a given URL in the browser
  @available(iOS, deprecated: 15.0, message: "Use `openURL` in conjunction with `OpenURLAction`")
  var openWalletURLAction: OpenWalletURLAction? {
    get { self[OpenWalletURLActionKey.self] }
    set { self[OpenWalletURLActionKey.self] = newValue }
  }
}
