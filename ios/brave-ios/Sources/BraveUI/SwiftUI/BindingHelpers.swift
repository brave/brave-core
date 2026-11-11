// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension Optional {
  /// Whether or not the optional currently holds a value.
  ///
  /// Setting this to `false` clears the value. Setting it to `true` has no effect since there is no
  /// value to create.
  ///
  /// Useful for driving presentation from an optional through `Binding`'s dynamic member lookup:
  ///
  ///     .sheet(isPresented: $selectedItem.isPresented) { ... }
  public var isPresented: Bool {
    get { self != nil }
    set {
      if !newValue {
        self = nil
      }
    }
  }
}

extension Set {
  /// Whether or not the set contains `member`.
  ///
  /// Setting this to `true` inserts the member and setting it to `false` removes it.
  ///
  /// Useful for binding a toggle to set membership through `Binding`'s dynamic member lookup:
  ///
  ///     Toggle(isOn: $selectedIDs[contains: item.id]) { ... }
  public subscript(contains member: Element) -> Bool {
    get { contains(member) }
    set {
      if newValue {
        insert(member)
      } else {
        remove(member)
      }
    }
  }
}
