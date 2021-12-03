// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import CoreServices

extension UIPasteboard {
  /// Copies a string into the pasteboard that is excluded from Handoff and expires at a certain date
  ///
  /// Defaults to expiring a secure string after 60s
  func setSecureString(
    _ string: String,
    expirationDate: Date = Date().addingTimeInterval(60)
  ) {
    setItems(
      [[kUTTypeUTF8PlainText as String: string]],
      options: [
        .localOnly: true,
        .expirationDate: expirationDate
      ]
    )
  }
}
