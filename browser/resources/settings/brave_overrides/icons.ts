// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import '../icons.html.js'
import '../brave_icons.html.js'

import {OverrideIronIcons} from 'chrome://resources/brave/polymer_overriding.js'

OverrideIronIcons('settings', 'brave_settings', {
  language: 'language',
  performance: 'performance'
})
OverrideIronIcons('cr', 'brave_settings', {
  security: 'privacy-security',
  search: 'search-engine',
  ['file-download']: 'download',
  print: 'printing'
})
OverrideIronIcons('settings20', 'brave_settings20', {
  incognito: 'private-mode'
})
