// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

/// For adding a sample to an enumerated histogram
public func UmaHistogramEnumeration<E: RawRepresentable & CaseIterable>(
  _ name: String,
  sample: E
) where E.RawValue == Int {
  UmaHistogramExactLinear(name, sample.rawValue, E.allCases.count + 1)
}
