// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

type MaybeObj = Object | any[] | null | undefined | number | string

export function objectEquals (x: MaybeObj, y: MaybeObj): boolean {
  // check falsey values
  if (x === null || x === undefined || y === null || y === undefined) {
    return x === y
  }

  // after this just checking type of one would be enough
  if (x.constructor !== y.constructor) {
    return false
  }

  // if they are functions, they should exactly refer to same one (because of closures)
  if (x instanceof Function) {
    return x === y
  }

  // if they are regexps, they should exactly refer to same one (it is hard to better equality check on current ES)
  if (x instanceof RegExp) {
    return x === y
  }

  // check valueOf (Dates, strings, numbers)
  if (x === y || x.valueOf() === y.valueOf()) {
    return true
  }

  // check array lengths
  if (
    Array.isArray(x) &&
    Array.isArray(y) &&
    x.length !== y.length
  ) {
    return false
  }

  // if they are dates, they must have had equal valueOf
  if (x instanceof Date) { return false }

  // if they are strictly equal, they both need to be object at least
  if (!(x instanceof Object)) { return false }
  if (!(y instanceof Object)) { return false }

  // recursive object equality check
  const xKeys = Object.keys(x)
  const yKeys = Object.keys(y)
  return (
    yKeys.every((yKey) => xKeys.includes(yKey)) &&
    xKeys.every((xKey) => objectEquals(x[xKey], y[xKey]))
  )
}
