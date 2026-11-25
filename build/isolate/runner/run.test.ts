// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Runs with the Node.js test runner rather than Jest, which can't load
// modules that use `import.meta`.

import assert from 'node:assert/strict'
import childProcess, { spawnSync } from 'node:child_process'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import process from 'node:process'
import { afterEach, beforeEach, describe, it, mock } from 'node:test'
import type { IsolateManifest, IsolateSuite } from './manifest.ts'
import { runIsolate } from './run.ts'

const outDir = path.resolve('/isolate/data/src/out/Release')

function makeSuite(
  name: string,
  overrides: Partial<IsolateSuite> = {},
): IsolateSuite {
  return {
    name,
    cwd: outDir,
    binary: name,
    args: [`--${name}`],
    envDefaults: {},
    ...overrides,
  }
}

function makeManifest(suites: IsolateSuite[]): IsolateManifest {
  return { suites }
}

type SpawnResult = Partial<ReturnType<typeof childProcess.spawnSync>>

function run(
  manifest: IsolateManifest,
  {
    results = {},
    extraArgs = [],
    env = {},
  }: {
    // Results of spawnSync by binary name; exit code 0 by default.
    results?: Record<string, SpawnResult>
    extraArgs?: string[]
    env?: NodeJS.ProcessEnv
  } = {},
) {
  const spawnSyncMock = mock.method(
    childProcess,
    'spawnSync',
    (command: string) =>
      (results[path.basename(command)] ?? { status: 0 }) as ReturnType<
        typeof childProcess.spawnSync
      >,
  )
  const logMock = mock.method(console, 'log', () => {})
  const errorMock = mock.method(console, 'error', () => {})
  try {
    const status = runIsolate({ manifest, extraArgs, env })
    const calls = spawnSyncMock.mock.calls.map((call) => call.arguments)
    const errors = errorMock.mock.calls.map((call) => call.arguments[0])
    return { status, calls, errors }
  } finally {
    spawnSyncMock.mock.restore()
    logMock.mock.restore()
    errorMock.mock.restore()
  }
}

describe('runIsolate', () => {
  it('runs each suite from its cwd with extra args appended', () => {
    const { status, calls } = run(makeManifest([makeSuite('a')]), {
      extraArgs: ['--x'],
    })
    const cwd = outDir
    assert.equal(status, 0)
    assert.deepEqual(calls, [
      [path.join(cwd, 'a'), ['--a', '--x'], { cwd, env: {}, stdio: 'inherit' }],
    ])
  })

  it('stops at the first failing suite and returns its exit code', () => {
    const { status, calls, errors } = run(
      makeManifest([makeSuite('a'), makeSuite('b'), makeSuite('c')]),
      { results: { b: { status: 3 } } },
    )
    assert.equal(status, 3)
    assert.deepEqual(
      calls.map(([command]) => path.basename(command)),
      ['a', 'b'],
    )
    assert.deepEqual(errors, ['b failed with exit code 3'])
  })

  it('reports a suite terminated by a signal once', () => {
    const { status, errors } = run(makeManifest([makeSuite('a')]), {
      results: { a: { status: null, signal: 'SIGKILL' } },
    })
    assert.equal(status, 128 + 9)
    assert.deepEqual(errors, ['a was terminated by signal SIGKILL'])
  })

  it('reports a suite that fails to start once', () => {
    const { status, errors } = run(makeManifest([makeSuite('a')]), {
      results: { a: { error: new Error('spawn ENOENT') } },
    })
    assert.equal(status, 1)
    assert.deepEqual(errors, ['a failed to start: spawn ENOENT'])
  })

  it('applies env defaults only when unset or empty', () => {
    const { calls } = run(
      makeManifest([
        makeSuite('a', { envDefaults: { A: 'default', B: 'default' } }),
      ]),
      { env: { A: 'host', B: '', C: 'host' } },
    )
    assert.deepEqual(calls[0]?.[2]?.env, { A: 'host', B: 'default', C: 'host' })
  })
})

describe('run.ts', () => {
  it('requires a manifest path', () => {
    const result = spawnSync(
      process.execPath,
      [path.join(import.meta.dirname, 'run.ts')],
      { encoding: 'utf8' },
    )
    assert.equal(result.status, 1)
    assert.match(result.stderr, /Usage: node run\.ts <manifest\.json>/)
  })
})

describe('run.sh', { skip: process.platform === 'win32' }, () => {
  let isolateDir: string
  let runnerDir: string

  beforeEach(() => {
    isolateDir = fs.mkdtempSync(path.join(os.tmpdir(), 'isolate-runner-'))
    runnerDir = path.join(isolateDir, 'runner')
    fs.cpSync(import.meta.dirname, runnerDir, { recursive: true })
    fs.copyFileSync(
      path.join(import.meta.dirname, '..', 'run.sh'),
      path.join(isolateDir, 'run.sh'),
    )
    // The runner must not depend on package.json files of parent directories.
    fs.writeFileSync(
      path.join(isolateDir, 'package.json'),
      '{ "type": "commonjs" }',
    )

    const outDir = path.join(isolateDir, 'data', 'src', 'out', 'Release')
    fs.mkdirSync(outDir, { recursive: true })
    fs.writeFileSync(
      path.join(outDir, 'fake_tests'),
      '#!/bin/sh\necho "$@" > args.txt\nexit "${FAKE_STATUS:-0}"\n',
      { mode: 0o755 },
    )
    fs.writeFileSync(
      path.join(isolateDir, 'manifest.json'),
      JSON.stringify(
        makeManifest([
          makeSuite('fake_tests', {
            cwd: 'data/src/out/Release',
            args: ['--a'],
          }),
        ]),
      ),
    )
  })

  afterEach(() => {
    fs.rmSync(isolateDir, { recursive: true, force: true })
  })

  function runSh(env: NodeJS.ProcessEnv = {}) {
    return spawnSync(path.join(isolateDir, 'run.sh'), ['--extra'], {
      encoding: 'utf8',
      env: { ...process.env, ...env },
    })
  }

  it('runs the suites with the given args', () => {
    const result = runSh()
    assert.equal(result.status, 0, result.stderr)
    assert.equal(
      fs.readFileSync(
        path.join(isolateDir, 'data/src/out/Release/args.txt'),
        'utf8',
      ),
      '--a --extra\n',
    )
  })

  it('returns the exit code of a failed suite', () => {
    assert.equal(runSh({ FAKE_STATUS: '3' }).status, 3)
  })
})
