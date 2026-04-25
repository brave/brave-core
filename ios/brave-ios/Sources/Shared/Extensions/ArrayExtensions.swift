// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension Array {

  // Performs a union operator using the result of f(Element) as the value to base uniqueness on.
  public func union<T: Hashable>(_ arr: [Element], f: ((Element) -> T)) -> [Element] {
    let result = self + arr
    return result.unique(f)
  }

  // Returns unique values in an array using the result of f()
  public func unique<T: Hashable>(_ f: ((Element) -> T)) -> [Element] {
    var map: [T: Element] = [T: Element]()
    return self.compactMap { a in
      let t = f(a)
      if map[t] == nil {
        map[t] = a
        return a
      } else {
        return nil
      }
    }
  }

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

  /// Returns all elements up until a condition is no longer satisfied.
  public func contiguousUntil(condition: (Element) -> Bool) -> ArraySlice<Element> {
    let index = firstIndex(where: { !condition($0) }) ?? self.count
    return self[0..<index]
  }
}

extension Collection {
  /// Returns the element at the specified index iff it is within bounds, otherwise nil.
  public subscript(safe index: Index) -> Element? {
    return indices.contains(index) ? self[index] : nil
  }
}
