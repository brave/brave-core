// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {OverrideIronIcons} from 'chrome://brave-resources/polymer_overriding.js'

import '../icons.js'
import '../brave_icons.m.js'

OverrideIronIcons('settings', 'brave_settings', {
  palette: 'appearance',
  assignment: 'autofill',
  language: 'language',
  build: 'system',
  restore: 'reset-settings'
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
