// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { execSync } from 'child_process'

// Run the TypeScript compiler.
run('tsc --project build/commands')

// Run the ESLint linter. This is required to catch possible runtime errors
// that might occur because of broken imports.
run('eslint --quiet build/commands')

// Run the Jest test runner. Pass through any extra arguments to it.
const extraArgs = process.argv.slice(2).join(' ')
run(`jest build/commands${extraArgs ? ` ${extraArgs}` : ''}`)

// Run a command and exit cleanly without exception noise.
function run(cmd: string): void {
  console.log(`> ${cmd}`)
  try {
    execSync(cmd, { stdio: 'inherit' })
  } catch (e: any) {
    process.exit(e.status ?? 1)
  }
}
