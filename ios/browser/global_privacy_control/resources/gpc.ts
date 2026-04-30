// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

Object.defineProperty(navigator, 'globalPrivacyControl', {
  enumerable: false,
  configurable: false,
  writable: false,
  value: (window as any).gCrWebPlaceholderGPCEnabled
})
