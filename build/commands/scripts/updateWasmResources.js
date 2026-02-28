/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import path from 'node:path'
import config from '../lib/config.js'
import util from '../lib/util.js'

const args = [
  path.join(
    config.srcDir,
    'brave',
    'ui',
    'webui',
    'resources',
    'wasm',
    'update.py',
  ),
]
util.run('vpython3', args, config.defaultOptions)
