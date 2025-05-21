// Copyright (c) 2018 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const debounce = function <T extends unknown[]>(fn: (...args: T) => void, bufferInterval?: number) {
  let timeout: any
  return (...args: T) => {
    clearTimeout(timeout)
    timeout = setTimeout(fn.apply.bind(fn, this, args), bufferInterval)
  }
}
