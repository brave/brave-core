// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import './testPolyfills'

// Mock the locale module so we don't need to provide strings in tests
jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    return key
  },
  formatLocale: (key: string) => {
    return key
  }
}))

// Mock the string keys - this gets around the fact that `const enums` are
// compiled away by jest's typescript transform.
;(globalThis as any).S = new Proxy({}, {
  get: (target, prop) => {
    return prop
  }
})
