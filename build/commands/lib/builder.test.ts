// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// TODO(http://brave.dev/b/56449): Experimental implementation for integration
// with our bots generator.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { Command } from 'commander'
import {
  getBuilderBuildTarget,
  getBuilderGclientTargets,
  loadBuilder,
  supportBuilder,
} from './builder.ts'
import * as Log from './log.ts'

// `var`, as imports run before this initialiser and read `isCI` while loading.
// eslint-disable-next-line no-var
var mockIsCI = false

jest.mock('./ciDetect.ts', () => ({
  get isCI() {
    return mockIsCI
  },
}))

jest.mock('./log.ts', () => ({
  warn: jest.fn(),
}))

function writeBuilder(dir: string, name: string) {
  const builderDir = path.join(dir, name)
  fs.mkdirSync(builderDir, { recursive: true })
  fs.writeFileSync(
    path.join(builderDir, 'gn-args.json'),
    JSON.stringify({
      gn_args: { target_os: 'linux', target_cpu: 'x64', is_asan: true },
      secrets: {},
    }),
  )
  fs.writeFileSync(
    path.join(builderDir, 'sync.json'),
    JSON.stringify({ target_os: ['android'], target_cpu: [] }),
  )
  fs.writeFileSync(
    path.join(builderDir, 'targets.json'),
    JSON.stringify({ compile: ['brave:all'], tests: ['brave_unit_tests'] }),
  )
}

describe('loadBuilder', () => {
  let dir: string

  beforeEach(() => {
    dir = fs.mkdtempSync(path.join(os.tmpdir(), 'builders-'))
  })

  afterEach(() => {
    fs.rmSync(dir, { recursive: true, force: true })
  })

  it('loads the generated files', () => {
    writeBuilder(dir, 'linux-x64-asan-brave')
    expect(loadBuilder('linux-x64-asan-brave', dir)).toEqual({
      name: 'linux-x64-asan-brave',
      gnArgs: {
        gn_args: { target_os: 'linux', target_cpu: 'x64', is_asan: true },
        secrets: {},
      },
      sync: { target_os: ['android'], target_cpu: [] },
      targets: { compile: ['brave:all'], tests: ['brave_unit_tests'] },
    })
  })

  it('rejects an unknown builder, listing the known ones', () => {
    writeBuilder(dir, 'linux-x64-asan-brave')
    writeBuilder(dir, 'linux-x64-asan-chromium')
    expect(() => loadBuilder('nope', dir)).toThrow(
      'unknown builder: nope\n\navailable builders:\n'
        + '  linux-x64-asan-brave\n  linux-x64-asan-chromium',
    )
  })

  it.each(['../etc', 'Foo', 'a/b', '', '-a'])(
    'rejects the invalid name %j',
    (name) => {
      expect(() => loadBuilder(name, dir)).toThrow('invalid builder name')
    },
  )

  it('reports a missing file', () => {
    writeBuilder(dir, 'b')
    fs.rmSync(path.join(dir, 'b', 'sync.json'))
    expect(() => loadBuilder('b', dir)).toThrow('sync.json')
  })
})

describe('supportBuilder', () => {
  let dir: string

  beforeEach(() => {
    dir = fs.mkdtempSync(path.join(os.tmpdir(), 'builders-'))
    writeBuilder(dir, 'fake-builder')
  })

  afterEach(() => {
    mockIsCI = false
    fs.rmSync(dir, { recursive: true, force: true })
  })

  const run = (argv: string[], allowed?: string[]) => {
    const action = jest.fn()
    const command = new Command()
      .exitOverride()
      .configureOutput({ writeErr: () => {} })
      .argument('[build_config]')
      .option('--target_os <os>')
      .option('--no-history')
      .option('--init')
    supportBuilder(command, allowed ? { allowed, dir } : { dir })
    command.action(action).parse(argv, { from: 'user' })
    return { action }
  }

  it('does nothing without --builder', () => {
    const { action } = run(['Release', '--target_os', 'linux'])
    expect(action).toHaveBeenCalled()
    expect(Log.warn).not.toHaveBeenCalled()
  })

  it('warns that --builder is experimental', () => {
    run(['--builder', 'fake-builder'])
    expect(Log.warn).toHaveBeenCalledWith(
      '--builder is experimental and may change without notice.',
    )
  })

  it('is not allowed on CI', () => {
    mockIsCI = true
    expect(() => run(['--builder', 'fake-builder'])).toThrow(
      '--builder is experimental and not allowed on CI',
    )
  })

  it('is not restricted on CI when --builder is absent', () => {
    mockIsCI = true
    const { action } = run(['Release'])
    expect(action).toHaveBeenCalled()
  })

  it('rejects an unknown builder', () => {
    expect(() => run(['--builder', 'nope'])).toThrow(
      'unknown builder: nope\n\navailable builders:\n  fake-builder',
    )
  })

  it('hands the loaded builder to the action', () => {
    const { action } = run(['--builder', 'fake-builder'])
    expect(action.mock.calls[0]![1].builder).toEqual(
      loadBuilder('fake-builder', dir),
    )
  })

  it('rejects other flags and positional arguments', () => {
    expect(() =>
      run([
        'Release',
        '--builder',
        'fake-builder',
        '--target_os',
        'linux',
        '--no-history',
      ]),
    ).toThrow(
      '--builder cannot be combined with: --target_os --no-history Release',
    )
  })

  it('allows the listed options', () => {
    const { action } = run(['--builder', 'fake-builder', '--init'], ['init'])
    expect(action).toHaveBeenCalled()
  })
})

const fakeBuilder = (
  gnArgs: Record<string, unknown>,
  sync: { target_os: string[]; target_cpu: string[] },
) => ({
  name: 'fake-builder',
  gnArgs: { gn_args: gnArgs },
  sync,
  targets: { compile: [], tests: [] },
})

describe('getBuilderGclientTargets', () => {
  it('returns the lists from sync.json, not the build target', () => {
    expect(
      getBuilderGclientTargets(
        fakeBuilder(
          { target_os: 'android', target_cpu: 'arm64' },
          { target_os: ['android', 'linux'], target_cpu: [] },
        ),
      ),
    ).toEqual({ targetOSList: ['android', 'linux'], targetArchList: [] })
  })

  it('leaves the lists empty to sync the host only', () => {
    expect(
      getBuilderGclientTargets(
        fakeBuilder(
          { target_os: 'linux', target_cpu: 'x64' },
          { target_os: [], target_cpu: [] },
        ),
      ),
    ).toEqual({ targetOSList: [], targetArchList: [] })
  })
})

describe('getBuilderBuildTarget', () => {
  it('builds for the target in the GN args', () => {
    expect(
      getBuilderBuildTarget(
        fakeBuilder(
          { target_os: 'linux', target_cpu: 'x64' },
          { target_os: [], target_cpu: [] },
        ),
        'linux',
      ),
    ).toEqual({ targetOS: 'linux', targetArch: 'x64' })
  })

  it('does not take the target from the gclient lists', () => {
    expect(
      getBuilderBuildTarget(
        fakeBuilder(
          { target_os: 'android', target_cpu: 'arm64' },
          { target_os: ['linux', 'android'], target_cpu: ['x64'] },
        ),
        'linux',
      ),
    ).toEqual({ targetOS: 'android', targetArch: 'arm64' })
  })

  it('rejects a target_os the checkout does not sync', () => {
    expect(() =>
      getBuilderBuildTarget(
        fakeBuilder(
          { target_os: 'android', target_cpu: 'arm64' },
          { target_os: [], target_cpu: [] },
        ),
        'linux',
      ),
    ).toThrow(
      "builder fake-builder builds for target_os android, which is neither the host's (linux) nor in its sync.json",
    )
  })

  it('does not require a cross target_cpu to be listed', () => {
    expect(
      getBuilderBuildTarget(
        fakeBuilder(
          { target_os: 'android', target_cpu: 'arm64' },
          { target_os: ['android'], target_cpu: [] },
        ),
        'linux',
      ),
    ).toEqual({ targetOS: 'android', targetArch: 'arm64' })
  })

  it('requires the build target in the GN args', () => {
    expect(() =>
      getBuilderBuildTarget(
        fakeBuilder({ target_os: 'linux' }, { target_os: [], target_cpu: [] }),
        'linux',
      ),
    ).toThrow('builder fake-builder: gn-args.json has no target_cpu')
  })
})
