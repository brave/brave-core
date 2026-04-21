// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'

import config from '../lib/config.ts'
import util from '../lib/util.js'

util.run(
  'vpython3',
  [path.join(config.srcDir, 'brave', 'third_party', 'boringtun', 'update.py')],
  config.defaultOptions,
)
