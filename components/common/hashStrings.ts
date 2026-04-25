// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

function charsum (s: string) {
  let sum = 0
  for (let i = 0; i < s.length; i++) {
    sum += (s.charCodeAt(i) * (i + 1))
  }
  return sum
}

export default function hashStrings (items: string[]) {
  // https://stackoverflow.com/a/25105589
  let sum = 0
  let product = 1
  for (let i = 0; i < items.length; i++) {
    let cs = charsum(items[i])
    if (product % cs > 0) {
      product = product * cs
      sum = sum + (65027 / cs)
    }
  }
  return ('' + sum).slice(0, 16)
}
