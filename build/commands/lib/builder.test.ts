// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// TODO(http://brave.dev/b/40327): Experimental implementation for integration
// with our bots generator.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { Command } from 'commander'
import { loadBuilder, supportBuilder } from './builder.ts'

function writeBuilder(dir: string, name: string) {
  const builderDir = path.join(dir, name)
  fs.mkdirSync(builderDir, { recursive: true })
  fs.writeFileSync(
    path.join(builderDir, 'gn-args.json'),
    JSON.stringify({ gn_args: { is_asan: true }, secrets: {} }),
  )
  fs.writeFileSync(
    path.join(builderDir, 'sync.json'),
    JSON.stringify({
      target_os: 'linux',
      target_cpu: 'x64',
      gclient_overrides: {},
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
      gnArgs: { gn_args: { is_asan: true }, secrets: {} },
      sync: { target_os: 'linux', target_cpu: 'x64', gclient_overrides: {} },
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
  const run = (argv: string[], allowed?: string[]) => {
    const action = jest.fn()
    const command = new Command()
      .exitOverride()
      .configureOutput({ writeErr: () => {} })
      .argument('[build_config]')
      .option('--target_os <os>')
      .option('--no-history')
      .option('--init')
    supportBuilder(command, allowed)
    command.action(action).parse(argv, { from: 'user' })
    return { action }
  }

  it('does nothing without --builder', () => {
    const { action } = run(['Release', '--target_os', 'linux'])
    expect(action).toHaveBeenCalled()
  })

  it('rejects an unknown builder', () => {
    expect(() => run(['--builder', 'nope'])).toThrow(/unknown builder: nope/)
  })

  it('accepts --builder alone', () => {
    const { action } = run(['--builder', 'linux-x64-asan-brave'])
    expect(action).toHaveBeenCalled()
  })

  it('rejects other flags and positional arguments', () => {
    expect(() =>
      run([
        'Release',
        '--builder',
        'linux-x64-asan-brave',
        '--target_os',
        'linux',
        '--no-history',
      ]),
    ).toThrow(
      '--builder cannot be combined with: --target_os --no-history Release',
    )
  })

  it('allows the listed options', () => {
    const { action } = run(
      ['--builder', 'linux-x64-asan-brave', '--init'],
      ['init'],
    )
    expect(action).toHaveBeenCalled()
  })
})
