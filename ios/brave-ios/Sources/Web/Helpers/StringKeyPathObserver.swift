// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

class StringKeyPathObserver<Object: NSObject, Value>: NSObject {
  weak var object: Object?
  let keyPath: String
  let changeHandler: (Value?) -> Void
  var isInvalidated: Bool = false

  init(object: Object, keyPath: String, changeHandler: @escaping (Value?) -> Void) {
    self.object = object
    self.keyPath = keyPath
    self.changeHandler = changeHandler
    super.init()
    object.addObserver(self, forKeyPath: keyPath, options: .new, context: nil)
  }

  func invalidate() {
    if isInvalidated { return }
    defer { isInvalidated = true }
    object?.removeObserver(self, forKeyPath: keyPath)
  }

  deinit {
    invalidate()
  }

  override func observeValue(
    forKeyPath keyPath: String?,
    of object: Any?,
    change: [NSKeyValueChangeKey: Any]?,
    context: UnsafeMutableRawPointer?
  ) {
    if self.keyPath != keyPath { return }
    let newValue = change?[.newKey] as? Value
    changeHandler(newValue)
  }
}
