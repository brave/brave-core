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
  type Builder,
  type BuilderSpec,
  getBuilderBuildTarget,
  getBuilderGclientTargets,
  loadBuilder,
  supportBuilder,
} from './builder.ts'
import config from './config.ts'
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

jest.mock('./config.ts', () => ({
  __esModule: true,
  default: { useBuilder: jest.fn() },
}))

const spec: BuilderSpec = {
  builder_group: 'brave.test',
  chromium_config: { target_platform: 'linux', build_config: 'Release' },
  gclient_config: { target_os: ['android'], target_cpu: [] },
}

// A `properties.json` with `spec` as *name*'s.
function properties(name: string): object {
  return {
    $brave_tests_builder_config: {
      builder_config: {
        builder_db: {
          entries: [{ builder_id: { builder: name }, builder_spec: spec }],
        },
        builder_ids: [{ builder: name }],
      },
    },
    builder_group: 'brave.test',
    recipe: 'brave_builder',
  }
}

function writeBuilder(dir: string, name: string): void {
  const builderDir = path.join(dir, name)
  fs.mkdirSync(builderDir, { recursive: true })
  fs.writeFileSync(
    path.join(builderDir, 'properties.json'),
    JSON.stringify(properties(name)),
  )
  fs.writeFileSync(
    path.join(builderDir, 'gn-args.json'),
    JSON.stringify({
      gn_args: { target_os: 'linux', target_cpu: 'x64', is_asan: true },
    }),
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
      spec,
      buildTarget: { targetOS: 'linux', targetArch: 'x64' },
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

  it('requires the build target in gn-args.json', () => {
    writeBuilder(dir, 'b')
    fs.writeFileSync(
      path.join(dir, 'b', 'gn-args.json'),
      JSON.stringify({ gn_args: { target_os: 'linux' } }),
    )
    expect(() => loadBuilder('b', dir)).toThrow(
      'builder b: gn-args.json has no target_cpu',
    )
  })

  it('requires its spec in properties.json', () => {
    writeBuilder(dir, 'b')
    fs.writeFileSync(
      path.join(dir, 'b', 'properties.json'),
      JSON.stringify(properties('other')),
    )
    expect(() => loadBuilder('b', dir)).toThrow(
      'builder b: properties.json has no spec for it',
    )
  })

  it('reports a missing file', () => {
    writeBuilder(dir, 'b')
    fs.rmSync(path.join(dir, 'b', 'properties.json'))
    expect(() => loadBuilder('b', dir)).toThrow('properties.json')
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
    expect(config.useBuilder).not.toHaveBeenCalled()
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

  it('makes config read .env as --builder allows', () => {
    run(['--builder', 'fake-builder'])
    expect(config.useBuilder).toHaveBeenCalledTimes(1)
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

describe('getBuilderGclientTargets', () => {
  const builder = (gclientConfig: BuilderSpec['gclient_config']): Builder => ({
    name: 'fake-builder',
    spec: {
      builder_group: 'brave.test',
      chromium_config: { target_platform: 'android' },
      gclient_config: gclientConfig,
    },
    buildTarget: { targetOS: 'android', targetArch: 'arm64' },
    targets: { compile: [], tests: [] },
  })

  it('returns the lists from properties.json', () => {
    expect(
      getBuilderGclientTargets(
        builder({ target_os: ['android', 'linux'], target_cpu: ['arm64'] }),
      ),
    ).toEqual({ targetOSList: ['android', 'linux'], targetArchList: ['arm64'] })
  })

  it('leaves the lists empty to sync the host only', () => {
    expect(
      getBuilderGclientTargets(builder({ target_os: [], target_cpu: [] })),
    ).toEqual({ targetOSList: [], targetArchList: [] })
  })

  it('builds for the target in gn-args.json, not the gclient lists', () => {
    expect(
      getBuilderBuildTarget(
        builder({ target_os: ['linux', 'android'], target_cpu: ['x64'] }),
      ),
    ).toEqual({ targetOS: 'android', targetArch: 'arm64' })
  })
})
