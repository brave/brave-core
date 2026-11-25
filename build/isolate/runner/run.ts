// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Runs the test suites of an isolate archive. See
// build/isolate/README.md in brave-core.
//
// Usage: node run.ts <manifest.json> [extra test args...]
// Prefer run.sh|run.bat, which check the environment first.

import childProcess from 'node:child_process'
import os from 'node:os'
import path from 'node:path'
import process from 'node:process'
import { readManifest, type IsolateManifest } from './manifest.ts'

type RunIsolateOptions = {
  manifest: IsolateManifest
  // Appended to the arguments of each suite.
  extraArgs: string[]
  env: NodeJS.ProcessEnv
}

// Runs the suites one by one, stopping at the first failure. Returns the exit
// code of the failed suite, or 0.
export function runIsolate(options: RunIsolateOptions): number {
  for (const suite of options.manifest.suites) {
    const binary = path.resolve(suite.cwd, suite.binary)
    const args = [...suite.args, ...options.extraArgs]
    const env = { ...options.env }
    for (const [name, value] of Object.entries(suite.envDefaults)) {
      if (!env[name]) {
        env[name] = value
      }
    }

    console.log(`Running ${suite.name}: ${[binary, ...args].join(' ')}`)
    // Called through the module, so that tests can mock it.
    const result = childProcess.spawnSync(binary, args, {
      cwd: suite.cwd,
      env,
      stdio: 'inherit',
    })
    if (result.error) {
      console.error(`${suite.name} failed to start: ${result.error.message}`)
      return 1
    }
    if (result.signal) {
      console.error(`${suite.name} was terminated by signal ${result.signal}`)
      // Like shells do, exit with 128 + the signal number.
      return 128 + os.constants.signals[result.signal]
    }
    if (result.status !== 0) {
      console.error(`${suite.name} failed with exit code ${result.status}`)
      return result.status ?? 1
    }
  }
  return 0
}

if (import.meta.main) {
  const [manifestFile, ...extraArgs] = process.argv.slice(2)
  if (!manifestFile) {
    console.error('Usage: node run.ts <manifest.json> [extra test args...]')
    process.exit(1)
  }
  process.exitCode = runIsolate({
    manifest: readManifest(manifestFile),
    extraArgs,
    env: process.env,
  })
}
