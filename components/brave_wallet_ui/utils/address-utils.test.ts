/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { isValidFilAddress } from './address-utils'

test('Valid FIL BLS account 1', () => {
return expect(isValidFilAddress('t3rvcqmc5otc3sh3cngqg2ttzcu7ezpco466lbafzaoygxvnzsw7e7n2zbjwhiv5fdzhs6uxm2qckwt6lp5wga')).toStrictEqual(true)
})

test('Valid FIL BLS account 2', () => {
return expect(isValidFilAddress('f3rvcqmc5otc3sh3cngqg2ttzcu7ezpco466lbafzaoygxvnzsw7e7n2zbjwhiv5fdzhs6uxm2qckwt6lp5wga')).toStrictEqual(true)
})

test('Valid FIL SECP account', () => {
return expect(isValidFilAddress('t1typmdwecdcidnwbrj67ogxut3kqcz57cb32o3iy')).toStrictEqual(true)
})

test('Valid FIL SECP account 2', () => {
return expect(isValidFilAddress('f1typmdwecdcidnwbrj67ogxut3kqcz57cb32o3iy')).toStrictEqual(true)
})

test('Invalid FIL SECP account 1', () => {
return expect(isValidFilAddress('ttypmdwecdcidnwbrj67ogxut3kqcz57cb32o3iy')).toStrictEqual(false)
})

test('Invalid FIL SECP account 2', () => {
return expect(isValidFilAddress('a1typmdwecdcidnwbrj67ogxut3kqcz57cb32o3iy')).toStrictEqual(false)
})

test('Invalid FIL account', () => {
return expect(isValidFilAddress('')).toStrictEqual(false)
})

test('Invalid FIL account 2', () => {
  return expect(isValidFilAddress('0x71C7656EC7ab88b098defB751B7401B5f6d8976F')).toStrictEqual(false)
})
