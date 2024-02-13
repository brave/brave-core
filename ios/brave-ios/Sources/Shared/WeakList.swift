/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

/**
 * A list that weakly holds references to its items.
 * Note that while the references themselves are cleared, their wrapper objects
 * are not (though they are reused). Also note that since slots are reused,
 * order is not preserved.
 *
 * This class crashes at runtime with EXC_BAD_ACCESS if a protocol is given as
 * the type T. Make sure to use a class type.
 */
open class WeakList<T: AnyObject>: Sequence {
  private var items = [WeakRef<T>]()

  public init() {}
  
  public init(_ items: some Collection<T>) {
    self.items = items.map { WeakRef($0) }
  }

  /**
     * Adds an item to the list.
     * Note that every insertion iterates through the list to find any "holes" (items that have
     * been deallocated) to reuse them, so this class may not be appropriate in situations where
     * insertion is frequent.
     */
  open func insert(_ item: T) {
    // Reuse any existing slots that have been deallocated.
    for wrapper in items where wrapper.value == nil {
      wrapper.value = item
      return
    }

    items.append(WeakRef(item))
  }

  open func makeIterator() -> AnyIterator<T> {
    var index = 0

    return AnyIterator() {
      if index >= self.items.count {
        return nil
      }

      for i in index..<self.items.count {
        if let value = self.items[i].value {
          index = i + 1
          return value
        }
      }

      index = self.items.count
      return nil
    }
  }

  open func count() -> Int {
    return self.items.count
  }

  open func index(of object: T) -> Int? {
    return self.items.firstIndex(where: { $0.value === object })
  }

  open subscript(index: Int) -> T? {
    return self.items[index].value
  }
}

open class WeakRef<T: AnyObject> {
  public weak var value: T?

  public init(_ value: T) {
    self.value = value
  }
}
