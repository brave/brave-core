/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import fs from 'node:fs'
import path from 'node:path'

import '../lib/checkEnvironment.js'

const nodeModules = path.join(process.cwd(), 'node_modules')

if (isNonPnpmNodeModules(nodeModules)) {
  console.log(`Removing non-pnpm node_modules at ${nodeModules}...`)
  fs.rmSync(nodeModules, { recursive: true, force: true })
}

function isPnpmNodeModules(nodeModulesDir: string): boolean {
  if (
    !fs.existsSync(nodeModulesDir)
    || !fs.statSync(nodeModulesDir).isDirectory()
  ) {
    return false
  }
  const pnpmDir = path.join(nodeModulesDir, '.pnpm')
  return fs.existsSync(pnpmDir) && fs.statSync(pnpmDir).isDirectory()
}

function isNonPnpmNodeModules(nodeModulesDir: string): boolean {
  return (
    fs.existsSync(nodeModulesDir)
    && fs.statSync(nodeModulesDir).isDirectory()
    && !isPnpmNodeModules(nodeModulesDir)
  )
}
