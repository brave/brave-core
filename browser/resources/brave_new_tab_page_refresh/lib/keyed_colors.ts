/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

function hashString(str: string) {
  let hash = 0
  for (let i = 0; i < str.length; ++i) {
    hash = Math.imul(31, hash) + str.charCodeAt(i)
  }
  return (hash | 0) + 2147483647 + 1
}

const colors = [
  '#FF9AA2',
  '#FFB7B2',
  '#FFDAC1',
  '#E2F0CB',
  '#B5EAD7',
  '#C7CEEA',
]

export function getKeyedColor(key: string) {
  return colors[hashString(key) % colors.length]
}
