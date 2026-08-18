// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

public enum ShieldsPanelAction {
  public enum NavigationTarget {
    case shareStats
    case reportBrokenSite
    case globalShields
  }
  case navigate(NavigationTarget, dismiss: Bool)
  case changedShieldSettings
  case shredSiteData
}
