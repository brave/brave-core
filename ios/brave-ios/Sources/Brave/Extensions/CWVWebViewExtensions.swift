// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

// Adds Sequence conformance to the `CWVBackForwardListItemArray` which under the hood is not
// actually an `NSArray` and only conforms to the `NSFastEnumeration` protocol
extension CWVBackForwardListItemArray: Sequence {
  public typealias Element = CWVBackForwardListItem

  // A custom Iterator is used because `NSFastEnumerationIterator.Element` is `Any`, and we can't
  // specialize it.
  public struct Iterator: IteratorProtocol {
    public typealias Element = CWVBackForwardListItem

    private var enumerator: NSFastEnumerationIterator

    fileprivate init(_ enumerable: CWVBackForwardListItemArray) {
      self.enumerator = NSFastEnumerationIterator(enumerable)
    }

    public mutating func next() -> Element? {
      return enumerator.next() as? Element
    }
  }

  public func makeIterator() -> Iterator {
    Iterator(self)
  }
}
