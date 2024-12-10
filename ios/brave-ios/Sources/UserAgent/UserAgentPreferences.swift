// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import UIKit

extension Preferences {
  public enum UserAgent {
    /// Sets Desktop UA for iPad by default (iOS 13+ & iPad only).
    /// Do not read it directly, prefer to use `UserAgent.shouldUseDesktopMode` instead.
    ///
    /// - Note: This preference is deprecated and no longer respected when using Chromium web views
    public static let alwaysRequestDesktopSite = Option<Bool>(
      key: "general.always-request-desktop-site",
      default: UIDevice.current.userInterfaceIdiom == .pad
    )
  }
}
