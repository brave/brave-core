// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Growth
import Shared

/// An environment containing dependencies that may be needed for indiviudal steps
public struct OnboardingEnvironment {
  @WeakRef public var p3aUtils: BraveP3AUtils?
  @WeakRef public var attributionManager: AttributionManager?

  public init(
    p3aUtils: BraveP3AUtils? = nil,
    attributionManager: AttributionManager? = nil
  ) {
    self._p3aUtils = WeakRef(wrappedValue: p3aUtils)
    self._attributionManager = WeakRef(wrappedValue: attributionManager)
  }
}
