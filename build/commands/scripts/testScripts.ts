// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { spawnSync } from 'node:child_process'

// Run the TypeScript compiler.
run('tsc', ['--project', './build'])

// The isolate runner has a stricter config: Node.js built-in types only.
run('tsc', ['--project', './build/isolate/runner'])

// Run the ESLint linter. This is required to catch possible runtime errors
// that might occur because of broken imports.
run('eslint', ['--quiet', './build'])

// Run the isolate runner tests. They use the Node.js test runner, as the
// isolate runner may only depend on Node.js built-ins.
run('node', ['--test', 'build/isolate/runner/*.test.ts'])

// Run the Jest test runner. Pass through any extra arguments to it.
run('jest', ['--roots', './build', ...process.argv.slice(2)])

// Run a command and exit on error.
function run(cmd: string, args: string[]): void {
  console.log(`> ${cmd} ${args.join(' ')}`)
  if (process.platform === 'win32') {
    args = ['/c', cmd, ...args]
    cmd = 'cmd'
  }
  const result = spawnSync(cmd, args, { stdio: 'inherit' })
  if (result.status !== 0) {
    process.exit(result.status ?? 1)
  }
}
