// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A helper data structure to help with counting aggregated data.
public struct CountableEntity: Hashable, Equatable {
  public let name: String
  public let count: Int

  public func hash(into hasher: inout Hasher) {
    hasher.combine(name)
  }

  public static func == (lhs: CountableEntity, rhs: CountableEntity) -> Bool {
    return lhs.name == rhs.name
  }
}
