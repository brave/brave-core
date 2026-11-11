// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/// <reference types="jest" />
/// <reference types="@testing-library/jest-dom" />

declare module 'chromedriver'

// Used in tests
declare namespace NodeJS {
  interface Global {
    requestAnimationFrame: any
    chrome: any
    window: any
    HTMLElement: any
    navigator: any
    document: any
  }
}
