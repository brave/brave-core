// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension Array {

  /// Splits an array into smaller arrays.
  /// For example `[1, 2, 3 ,4 ,5 ,6].splitEvery(3)`
  /// results in `[[1, 2, 3], [4, 5, 6]]`
  public func splitEvery(_ n: Int) -> [[Element]] {
    if n <= 0 || isEmpty { return [] }
    if n >= count { return [self] }

    return stride(from: 0, to: self.count, by: n).map {
      Array(self[$0..<Swift.min($0 + n, self.count)])
    }
  }
}

extension Collection {
  /// Returns the element at the specified index iff it is within bounds, otherwise nil.
  public subscript(safe index: Index) -> Element? {
    return indices.contains(index) ? self[index] : nil
  }
}
