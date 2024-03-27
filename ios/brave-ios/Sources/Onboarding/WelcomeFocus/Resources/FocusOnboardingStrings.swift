// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public struct FocusOnboarding {
    public static let continueButtonTitle = NSLocalizedString(
      "focusOnboarding.contextLimitErrorTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value:
        "Continue",
      comment:
        "The title of the button that changes the screen to next"
    )
  }
}
