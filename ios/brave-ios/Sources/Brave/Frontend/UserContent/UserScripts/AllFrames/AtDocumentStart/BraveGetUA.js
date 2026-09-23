// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

Object.defineProperty(navigator, 'brave', {
  enumerable: false,
  configurable: true,
  writable: false,
  value: Object.freeze({
    isBrave: () => new Promise(res => res(true))
  })
})
