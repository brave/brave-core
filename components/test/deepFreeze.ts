// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Deprecated: only use for tests to throw errors when
// something which is meant to be immutable is attempted to be mutated.
// For performance reasons, don't use in production, instead just declare a
// type as Readonly<T> to get compile errors.
export default function deepFreeze<T extends Object> (o: T): Readonly<T> {
  Object.freeze(o)
  if (o === undefined) {
    return o
  }

  Object.getOwnPropertyNames(o).forEach(function (prop) {
    if (o[prop] !== null
    && (typeof o[prop] === 'object' || typeof o[prop] === 'function')
    && !Object.isFrozen(o[prop])) {
      deepFreeze(o[prop])
    }
  })

  return o
}
