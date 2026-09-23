// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Trezor popup page is opened with window.open(url) which doesn't
// work(opened window loses opener) from chrome-untrusted:// pages
// (namely chrome-untrusted://trezor-bridge). We need to revert to legacy
// behavior which works: open blank page then navigate.
// So effectively we rollback this change:
// https://github.com/trezor/trezor-suite/pull/10975/changes#diff-38dd02260cff108b8329d6f3adbf8717b8e8737222de87a92c048f8bbf0bf159R256-R258
// by overwriting
// window.open(url, ...args)
// with
// window.open('', ...args).location.href = url
window.open = new Proxy(window.open, {
  apply(target, thisArg, [url, ...rest]) {
    const urlObj = URL.parse(url)
    if (urlObj && urlObj.origin === 'https://connect.trezor.io') {
      const result = Reflect.apply(target, thisArg, ['', ...rest])
      if (result && url) {
        result.location.href = url
      }
      return result
    }

    // Use the default window.open behavior for non-matching URLs
    return Reflect.apply(target, thisArg, [url, ...rest])
  },
})
