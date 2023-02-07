// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

public enum BraveSearchPromotionState: Int {
  /// Unknown state, this is default state when user didnt engage with promotion
  case undetermined = 0
  /// The user has pressed maybe later and didnt end the session
  case maybeLaterSameSession
  /// The user has pressed maybe later in a previous session but not pressed dismiss
  case maybeLaterUpcomingSession
  /// The user has dismissed brave search promo
  case dismissed
}
