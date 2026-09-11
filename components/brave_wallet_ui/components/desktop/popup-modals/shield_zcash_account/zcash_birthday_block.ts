// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const MIN_ACCOUNT_BIRTHDAY_BLOCK = 1687104

const ALLOWED_BIRTHDAY_BLOCK_INPUT = /^(?:|[1-9]\d*)$/
const BLOCKED_BIRTHDAY_BLOCK_KEYS = new Set(['.', ',', '-', '+', 'e', 'E'])

export const isAllowedZCashBirthdayBlockInput = (value: string): boolean => {
  return ALLOWED_BIRTHDAY_BLOCK_INPUT.test(value)
}

export const shouldBlockZCashBirthdayBlockKey = (
  key: string,
  currentValue: string,
): boolean => {
  if (BLOCKED_BIRTHDAY_BLOCK_KEYS.has(key)) {
    return true
  }
  return key === '0' && currentValue === ''
}

export type ZCashBirthdayBlockError = 'not-whole' | 'too-low' | 'too-high'

export const getZCashBirthdayBlockError = (
  customBirthdayBlock: string,
  chainTip?: number,
): ZCashBirthdayBlockError | undefined => {
  if (customBirthdayBlock === '') {
    return undefined
  }

  const accountBirthdayBlock = Number(customBirthdayBlock)
  if (!Number.isInteger(accountBirthdayBlock)) {
    return 'not-whole'
  }
  if (accountBirthdayBlock < MIN_ACCOUNT_BIRTHDAY_BLOCK) {
    return 'too-low'
  }
  if (chainTip !== undefined && accountBirthdayBlock > chainTip) {
    return 'too-high'
  }
  return undefined
}
