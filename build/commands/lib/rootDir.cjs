// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This is a CommonJS module so we can use __dirname to resolve the root
// directory. The rest of the codebase is ESM, which would require
// import.meta.dirname for this, but that isn't supported by Jest.

const fs = require('node:fs')
const path = require('node:path')

let dirName = __dirname
// Use fs.realpathSync to normalize the path(__dirname could be c:\.. or C:\..).
if (process.platform === 'win32') {
  dirName = fs.realpathSync.native(dirName)
}

const rootDir = path.resolve(dirName, '..', '..', '..', '..', '..')
if (rootDir.includes(' ')) {
  console.error(
    `Root directory contains spaces, this is not supported: ${rootDir}`,
  )
  process.exit(1)
}

module.exports = rootDir
