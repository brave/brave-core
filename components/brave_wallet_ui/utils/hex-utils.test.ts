// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { hexStrToNumberArray, numberArrayToHexStr } from './hex-utils'

const hex = 'bc614e'
const numberArray: number[] = [188, 97, 78]

describe('HexStrToNumberArray', () => {
  it('should convert hexadecimal to number array', () => {
    expect(hexStrToNumberArray(hex)).toEqual(numberArray)
  })

  it('should hexadecimal string starting with 0x to number array', () => {
    const hexString = '0x' + hex
    expect(hexStrToNumberArray(hexString)).toEqual(numberArray)
  })

  it('should return an empty array if value parameter is an empty string', () => {
    expect(hexStrToNumberArray('')).toEqual([])
  })
})

describe('numberArrayToHexStr', () => {
  it('should convert number array to hexadecimal string', () => {
    expect(numberArrayToHexStr(numberArray)).toEqual(hex)
  })

  it('should return an empty string if value parameter is an empty array', () => {
    expect(numberArrayToHexStr([])).toEqual('')
  })
})
