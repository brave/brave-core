/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const hexStrToNumberArray = (value: string): number[] => {
  if (!value) {
    return []
  }

  const hexStr = value.startsWith('0x') ? value.slice(2) : value
  const array = []
  for (let n = 0; n < hexStr.length; n += 2) {
    array.push(parseInt(hexStr.substr(n, 2), 16))
  }
  return array
}

export const numberArrayToHexStr = (value: number[]): string => {
  return value.map(x => x.toString(16).padStart(2, '0')).join('')
}
