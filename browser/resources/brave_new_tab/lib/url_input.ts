/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export function urlFromInput(input: string) {
  if (/\s/.test(input)) {
    return null
  }
  const bits = input.split('.')
  if (bits.length <= 1 || bits.join('').length === 0) {
    return null
  }
  if (!input.includes('://')) {
    input = `https://${input}`
  }
  const schemes = new Set(['http:', 'https:'])
  try {
    const url = new URL(input)
    if (!schemes.has(url.protocol)) {
      return null
    }
    return url
  } catch {
    return null
  }
}
