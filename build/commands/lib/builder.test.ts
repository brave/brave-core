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
  getBuilderGclientTargets,
  loadBuilder,
  supportBuilder,
} from './builder.ts'
import {
  mockBuilderProperties,
  mockBuilderSpec,
  writeMockBuilder,
} from './builderMock.ts'
import type { Builder, BuilderSpec } from './builderTypes.ts'
import config, { Config } from './config.ts'
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

describe('loadBuilder', () => {
  let dir: string

  beforeEach(() => {
    dir = fs.mkdtempSync(path.join(os.tmpdir(), 'builders-'))
  })

  afterEach(() => {
    fs.rmSync(dir, { recursive: true, force: true })
  })

  it('loads the generated files', () => {
    writeMockBuilder(dir, 'fake-builder')
    expect(loadBuilder('fake-builder', dir)).toEqual({
      name: 'fake-builder',
      spec: mockBuilderSpec,
      buildTarget: { targetOS: 'linux', targetArch: 'x64' },
      targets: { compile: ['brave:all'], tests: ['brave_unit_tests'] },
    })
  })

  it('rejects an unknown builder, listing the known ones', () => {
    writeMockBuilder(dir, 'fake-builder')
    writeMockBuilder(dir, 'other-builder')
    expect(() => loadBuilder('nope', dir)).toThrow(
      'unknown builder: nope\n\navailable builders:\n'
        + '  fake-builder\n  other-builder',
    )
  })

  it.each(['../etc', 'Foo', 'a/b', '', '-a'])(
    'rejects the invalid name %j',
    (name) => {
      expect(() => loadBuilder(name, dir)).toThrow('invalid builder name')
    },
  )

  it('requires the build target in gn-args.json', () => {
    writeMockBuilder(dir, 'b')
    fs.writeFileSync(
      path.join(dir, 'b', 'gn-args.json'),
      JSON.stringify({ gn_args: { target_os: 'linux' } }),
    )
    expect(() => loadBuilder('b', dir)).toThrow(
      'builder b: gn-args.json has no target_cpu',
    )
  })

  it('requires its spec in properties.json', () => {
    writeMockBuilder(dir, 'b')
    fs.writeFileSync(
      path.join(dir, 'b', 'properties.json'),
      JSON.stringify(mockBuilderProperties('other')),
    )
    expect(() => loadBuilder('b', dir)).toThrow(
      'builder b: properties.json has no spec for it',
    )
  })

  it('reports a missing file', () => {
    writeMockBuilder(dir, 'b')
    fs.rmSync(path.join(dir, 'b', 'properties.json'))
    expect(() => loadBuilder('b', dir)).toThrow('properties.json')
  })
})

describe('supportBuilder', () => {
  let dir: string
  let load: jest.SpiedFunction<typeof Config.load>

  beforeEach(() => {
    dir = fs.mkdtempSync(path.join(os.tmpdir(), 'builders-'))
    writeMockBuilder(dir, 'fake-builder')
    const loadConfig = Config.load.bind(Config)
    load = jest
      .spyOn(Config, 'load')
      .mockImplementation((builder) => loadConfig(builder, {}))
  })

  afterEach(() => {
    mockIsCI = false
    load.mockRestore()
    fs.rmSync(dir, { recursive: true, force: true })
  })

  const run = (argv: string[]) => {
    const action = jest.fn()
    const command = new Command()
      .exitOverride()
      .configureOutput({ writeErr: () => {} })
      .argument('[build_config]')
      .option('--target_os <os>')
    supportBuilder(command, { dir })
    command.action(action).parse(argv, { from: 'user' })
    return { action }
  }

  it('does nothing without --builder', () => {
    const { action } = run(['Release', '--target_os', 'linux'])
    expect(action).toHaveBeenCalled()
    expect(Log.warn).not.toHaveBeenCalled()
    expect(load).not.toHaveBeenCalled()
  })

  it("installs the builder's config", () => {
    run(['--builder', 'fake-builder'])
    expect(load).toHaveBeenCalledWith(loadBuilder('fake-builder', dir))
    expect(config.isBuilderSet()).toBe(true)
    expect(config.buildConfig).toBe('Release')
  })

  it('warns that --builder is experimental', () => {
    run(['--builder', 'fake-builder'])
    expect(Log.warn).toHaveBeenCalledWith(
      '--builder is experimental and may change without notice.',
    )
  })

  it('is not supported on CI without a recipe', () => {
    mockIsCI = true
    expect(() => run(['--builder', 'fake-builder'])).toThrow(
      '--builder is experimental and not supported on CI without a recipe',
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
})

describe('getBuilderGclientTargets', () => {
  const builder = (gclientConfig: BuilderSpec['gclient_config']): Builder => ({
    name: 'fake-builder',
    spec: {
      builder_group: 'brave.test',
      chromium_config: { target_platform: 'android' },
      gclient_config: gclientConfig,
      brave_config: mockBuilderSpec.brave_config,
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
})
