// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Strings

// MARK: - SSL Certificate Viewer

extension Strings.CertificateViewer {
  public static let sha256Title = "SHA-256"
  public static let sha1Title = "SHA-1"
}

extension Strings {
  /// "BAT" or "BAT Points" depending on the region
  public static var batSymbol: String {
    return Preferences.Rewards.isUsingBAP.value == true ? "BAP" : "BAT"
  }
}
