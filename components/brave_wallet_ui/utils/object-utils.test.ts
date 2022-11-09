// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { objectEquals } from './object-utils'

const a = { a: 'text', b: [0, 1] }
const b = { a: 'text', b: [0, 1] }
const c = { a: 'text', b: 0 }
const d = { a: 'text', b: false }
const e = { a: 'text', b: [1, 0] }
const i = {
  a: 'text',
  c: {
    b: [1, 0]
  }
}
const j = {
  a: 'text',
  c: {
    b: [1, 0]
  }
}
const k = { a: 'text', b: null }
const l = { a: 'text', b: undefined }

it('should detect if both objects are null', () => {
  expect(objectEquals(null, null)).toBe(true)
})

it('should fail if one obj is null and the other is undefined', () => {
  expect(objectEquals(null, undefined)).toBe(false)
})

it('should fail if both objs are different RexExp instances', () => {
  expect(objectEquals(/abc/, /abc/)).toBe(false)
  expect(objectEquals(/abc/, /123/)).toBe(false)
})

it('should pass if both objs are the same RexExp instances', () => {
  const r = /abc/
  expect(objectEquals(r, r)).toBe(true)
})

it('should detect string value differences', () => {
  expect(objectEquals('hi', 'hi')).toBe(true)
  expect(objectEquals('hi', 'yo')).toBe(false)
})

it('should detect numeric value differences', () => {
  expect(objectEquals(5, 5)).toBe(true)
  expect(objectEquals(5, 10)).toBe(false)
  // eslint-disable-next-line no-new-wrappers
  expect(objectEquals(new Number(5), 5)).toBe(true)
  // eslint-disable-next-line no-new-wrappers
  expect(objectEquals(new Number(5), 10)).toBe(false)
  // eslint-disable-next-line no-new-wrappers
  expect(objectEquals(new Number(1), '1')).toBe(false)
})

it('should detect differences in array values', () => {
  expect(objectEquals([], [])).toBe(true)
  expect(objectEquals([1, 2], [1, 2])).toBe(true)
  expect(objectEquals([1, 2], [2, 1])).toBe(false)
  expect(objectEquals([1, 2], [1, 2, 3])).toBe(false)
  expect(objectEquals([1, 2, undefined], [1, 2])).toBe(false)
})

it('should detect that an object with array-like keys is different than an array', () => {
  expect(objectEquals([1, 2, 3], { 0: 1, 1: 2, 2: 3 })).toBe(false)
})

it('should treat empty objects as equal', () => {
  expect(objectEquals({}, {})).toBe(true)
})

it('should detect differences in object property values', () => {
  expect(objectEquals({ a: 1, b: 2 }, { a: 1, b: 2 })).toBe(true)
  expect(objectEquals({ a: 1, b: 2 }, { b: 2, a: 1 })).toBe(true)
  expect(objectEquals({ a: 1, b: 2 }, { a: 1, b: 3 })).toBe(false)
  expect(objectEquals(a, b)).toBe(true)
  expect(objectEquals(a, c)).toBe(false)
  expect(objectEquals(c, d)).toBe(false)
  expect(objectEquals(a, e)).toBe(false)
})

it('should detect differences in nested object property values', () => {
  expect(
    objectEquals(
      { 1: { name: 'mhc', age: 28 }, 2: { name: 'arb', age: 26 } },
      { 1: { name: 'mhc', age: 28 }, 2: { name: 'arb', age: 26 } }
    )
  ).toBe(true)
  expect(
    objectEquals(
      { 1: { name: 'mhc', age: 28 }, 2: { name: 'arb', age: 26 } },
      { 1: { name: 'mhc', age: 28 }, 2: { name: 'arb', age: 27 } }
    )
  ).toBe(false)
  expect(objectEquals(i, j)).toBe(true)
  expect(objectEquals(d, k)).toBe(false)
  expect(objectEquals(k, l)).toBe(false)
})

it('should fail if an empty object is compared with null or undefined', () => {
  expect(objectEquals({}, null)).toBe(false)
  expect(objectEquals({}, undefined)).toBe(false)
})

it('should detect Date differences', () => {
  expect(objectEquals(new Date('2011-03-31'), new Date('2011-03-31'))).toBe(
    true
  )
  expect(objectEquals(new Date('2011-03-31'), new Date('1970-01-01'))).toBe(
    false
  )
  expect(objectEquals(new Date(1234), 1234)).toBe(false)
})

it('should always treat function as incomparable', () => {
  // no two different function is equal really, they capture their context variables
  // so even if they have same toString(), they won't have same functionality
  const func = function (x: any) {
    return true
  }
  const func2 = function (x: any) {
    return true
  }
  expect(objectEquals(func, func)).toBe(true)
  expect(objectEquals(func, func2)).toBe(false)
  expect(objectEquals({ a: { b: func } }, { a: { b: func } })).toBe(true)
  expect(objectEquals({ a: { b: func } }, { a: { b: func2 } })).toBe(false)
})
