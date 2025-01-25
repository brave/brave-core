/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export class Optional<T> {
  value_: T | undefined

  constructor (value?: T) {
    this.value_ = value
  }

  hasValue () {
    return this.value_ !== undefined
  }

  value () {
    if (this.value_ === undefined) {
      throw new Error('Cannot get value of empty optional')
    }
    return this.value_
  }

  valueOr<U> (fallback: U) {
    return this.value_ === undefined ? fallback : this.value_
  }
}

export function optional<T> (value?: T) {
  return new Optional<T>(value)
}
