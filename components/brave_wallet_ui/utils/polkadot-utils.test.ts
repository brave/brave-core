// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { bigIntToUint128, U128_MAX } from './polkadot-utils'

describe('bigIntToUint128', () => {
  it('all upper bits are zero', () => {
    const amount = bigIntToUint128(BigInt('0xffffffffffffffff'))
    expect(amount.high).toBe(BigInt(0))
    expect(amount.low).toBe(BigInt('0xffffffffffffffff'))
  })

  it('all lower bits are zero', () => {
    const amount = bigIntToUint128(BigInt('0xffffffffffffffff') << BigInt(64))
    expect(amount.high).toBe(BigInt('0xffffffffffffffff'))
    expect(amount.low).toBe(BigInt(0))
  })

  it('all bits are set', () => {
    const amount = bigIntToUint128(U128_MAX)
    expect(amount.high).toBe(BigInt('0xffffffffffffffff'))
    expect(amount.low).toBe(BigInt('0xffffffffffffffff'))
  })

  it('all bits are zero', () => {
    const amount = bigIntToUint128(BigInt(0))
    expect(amount.high).toBe(BigInt(0))
    expect(amount.low).toBe(BigInt(0))
  })

  it('should throw when numeric limits are exceeded', () => {
    const f = () => bigIntToUint128(U128_MAX + BigInt(1))
    expect(f).toThrowError('numeric limits exceeded for amount u128')
  })
})
