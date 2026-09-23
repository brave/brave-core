/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * This script removes the non-pnpm node_modules directory if it exists.
 */

import fs from 'node:fs'
import path from 'node:path'

import '../lib/checkEnvironment.js'

const nodeModulesDir = path.join(process.cwd(), 'node_modules')
const pnpmDir = path.join(nodeModulesDir, '.pnpm')

function isNonPnpmNodeModules(): boolean {
  return (
    fs.existsSync(nodeModulesDir)
    && fs.statSync(nodeModulesDir).isDirectory()
    && !fs.existsSync(pnpmDir)
  )
}

if (isNonPnpmNodeModules()) {
  console.log(`Removing non-pnpm node_modules at ${nodeModulesDir}...`)
  fs.rmSync(nodeModulesDir, { recursive: true, force: true })
}
