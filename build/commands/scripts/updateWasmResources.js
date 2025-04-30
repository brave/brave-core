/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const config = require('../lib/config')
const path = require('path')
const util = require('../lib/util')

args = [
  path.join(
    config.srcDir,
    'brave',
    'ui',
    'webui',
    'resources',
    'wasm',
    'update.py'
  )
]
util.run('vpython3', args, config.defaultOptions)
