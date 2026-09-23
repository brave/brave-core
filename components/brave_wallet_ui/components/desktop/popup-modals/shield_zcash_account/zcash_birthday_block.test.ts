// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  getZCashBirthdayBlockError,
  isAllowedZCashBirthdayBlockInput,
  MIN_ACCOUNT_BIRTHDAY_BLOCK,
  shouldBlockZCashBirthdayBlockKey,
} from './zcash_birthday_block'

const CHAIN_TIP = 7687104

describe('isAllowedZCashBirthdayBlockInput', () => {
  it('allows an empty value', () => {
    expect(isAllowedZCashBirthdayBlockInput('')).toBe(true)
  })

  it('allows a whole number without a leading zero', () => {
    expect(isAllowedZCashBirthdayBlockInput('1687104')).toBe(true)
  })

  it('rejects a decimal, sign, leading zero, or exponent', () => {
    expect(isAllowedZCashBirthdayBlockInput('1687104.5')).toBe(false)
    expect(isAllowedZCashBirthdayBlockInput('1687104.')).toBe(false)
    expect(isAllowedZCashBirthdayBlockInput('-1687104')).toBe(false)
    expect(isAllowedZCashBirthdayBlockInput('+1687104')).toBe(false)
    expect(isAllowedZCashBirthdayBlockInput('01687104')).toBe(false)
    expect(isAllowedZCashBirthdayBlockInput('0')).toBe(false)
    expect(isAllowedZCashBirthdayBlockInput('1e6')).toBe(false)
  })
})

describe('shouldBlockZCashBirthdayBlockKey', () => {
  it('blocks decimal, sign, and exponent keys', () => {
    expect(shouldBlockZCashBirthdayBlockKey('.', '1687104')).toBe(true)
    expect(shouldBlockZCashBirthdayBlockKey(',', '1687104')).toBe(true)
    expect(shouldBlockZCashBirthdayBlockKey('-', '')).toBe(true)
    expect(shouldBlockZCashBirthdayBlockKey('+', '')).toBe(true)
    expect(shouldBlockZCashBirthdayBlockKey('e', '1')).toBe(true)
    expect(shouldBlockZCashBirthdayBlockKey('E', '1')).toBe(true)
  })

  it('blocks a leading zero', () => {
    expect(shouldBlockZCashBirthdayBlockKey('0', '')).toBe(true)
    expect(shouldBlockZCashBirthdayBlockKey('0', '1687104')).toBe(false)
  })

  it('allows digits and editing keys', () => {
    expect(shouldBlockZCashBirthdayBlockKey('1', '')).toBe(false)
    expect(shouldBlockZCashBirthdayBlockKey('Backspace', '1687104')).toBe(false)
  })
})

describe('getZCashBirthdayBlockError', () => {
  it('allows an empty birthday so the default chain tip can be used', () => {
    expect(getZCashBirthdayBlockError('', CHAIN_TIP)).toBeUndefined()
  })

  it('allows a whole-number birthday in range', () => {
    expect(
      getZCashBirthdayBlockError(
        MIN_ACCOUNT_BIRTHDAY_BLOCK.toString(),
        CHAIN_TIP,
      ),
    ).toBeUndefined()
  })

  it('rejects a fractional birthday before range checks', () => {
    expect(getZCashBirthdayBlockError('1687104.5', CHAIN_TIP)).toBe('not-whole')
  })

  it('rejects a birthday below the minimum', () => {
    expect(
      getZCashBirthdayBlockError(
        (MIN_ACCOUNT_BIRTHDAY_BLOCK - 1).toString(),
        CHAIN_TIP,
      ),
    ).toBe('too-low')
  })

  it('rejects a birthday above the chain tip', () => {
    expect(
      getZCashBirthdayBlockError((CHAIN_TIP + 1).toString(), CHAIN_TIP),
    ).toBe('too-high')
  })
})
