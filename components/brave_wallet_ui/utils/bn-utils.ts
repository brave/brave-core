/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import BigNumber from 'bignumber.js'

/**
 * Returns the result of adding two numeric string amounts.
 *
 * The return value is always exact and unrounded. Do NOT use this
 * function with floating-point numeric values.
 *
 * @param a First numeric value.
 * @param b Second numeric value.
 */
export function addNumericValues (a: string, b: string): string {
  return new BigNumber(a).plus(b).toFixed(0)
}

/**
 * Returns the result of multiplying two numeric string amounts.
 *
 * The return value is always exact and unrounded. Do NOT use this
 * function with floating-point numeric values.
 *
 * @param a First numeric value.
 * @param b Second numeric value.
 */
export function multiplyNumericValues (a: string, b: string): string {
   return new BigNumber(a).times(b).toFixed(0)
}

/**
 * Returns the normalized string of the given numeric value.
 *
 * This function is typically used for converting a hex value to
 * numeric value.
 *
 * Invalid values return an empty string.
 *
 * @param value Numeric value to normalize.
 */
export function normalizeNumericValue (value: string): string {
  const valueBN = new BigNumber(value)
  return valueBN.isNaN()
    ? ''
    : valueBN.toFixed(0)
}

export function isNumericValueGreaterThan (a: string, b: string): boolean {
  return new BigNumber(a).isGreaterThan(b)
}
