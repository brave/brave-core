// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Storage
import Web

extension TabDataValues {
  private struct BraveUserAgentExceptionsKey: TabDataKey {
    static var defaultValue: BraveUserAgentExceptionsIOS?
  }

  /// A reference to a Brave user agent exceptions list that can be used to check
  /// if we should show Brave in the user agent for a given website.
  public var braveUserAgentExceptions: BraveUserAgentExceptionsIOS? {
    get { self[BraveUserAgentExceptionsKey.self] }
    set {
      if !FeatureList.kUseBraveUserAgent.enabled {
        return
      }
      self[BraveUserAgentExceptionsKey.self] = newValue
    }
  }
}
