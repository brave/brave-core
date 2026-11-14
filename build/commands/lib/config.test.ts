// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import os from 'node:os'
import { Command } from 'commander'
import { Config } from './config.ts'
import EnvConfig from './envConfig.ts'
import { supportNinjaOptions } from './buildOptions.ts'

function makeConfig(remote = false, args: string[] = []) {
  const config = new Config()
  config.sisoLimits = {}
  config.rbeService = ''
  config.use_no_gn_gen = false
  const command = supportNinjaOptions(new Command())
  command.parse(args, { from: 'user' })
  config.update({ use_remoteexec: remote, ...command.opts() })
  return config
}

function limits(config: Config) {
  return Object.fromEntries(
    (config.defaultOptions.env.SISO_LIMITS ?? '')
      .split(',')
      .filter(Boolean)
      .map((item) => item.split('=')),
  )
}

describe('Config Siso limits', () => {
  beforeEach(() => {
    jest.replaceProperty(process, 'env', {})
    const cpu = os.cpus()[0]!
    jest.spyOn(os, 'cpus').mockReturnValue(Array(8).fill(cpu))
  })

  afterEach(() => {
    jest.restoreAllMocks()
  })

  it.each([1, 6])('applies local --ninja=j:%i without RBE', (jobs) => {
    const config = makeConfig(false, [`--ninja=j:${jobs}`])
    expect(limits(config)).toEqual({ local: String(jobs) })
    expect(config.extraNinjaOpts).toEqual(['--offline'])
  })

  it('does not introduce a limit without a local override', () => {
    expect(makeConfig().defaultOptions.env.SISO_LIMITS).toBeUndefined()
  })

  it('leaves inherited limits untouched without a local override', () => {
    process.env.SISO_LIMITS = ' local=3,preproc=2,unknown=value '
    expect(makeConfig().defaultOptions.env.SISO_LIMITS).toBe(
      process.env.SISO_LIMITS,
    )
  })

  it.each([
    ['local=2', '2'],
    ['local=12', '6'],
  ])('caps inherited local limits %s', (inherited, expected) => {
    process.env.SISO_LIMITS = inherited
    expect(limits(makeConfig(false, ['--ninja=j:6']))).toEqual({
      local: expected,
    })
  })

  it('preserves other inherited pools when adding the local limit', () => {
    process.env.SISO_LIMITS = 'preproc=2,remote=9,custom=4'
    expect(limits(makeConfig(false, ['--ninja=j:6']))).toEqual({
      local: '6',
      preproc: '2',
      remote: '9',
      custom: '4',
    })
  })

  it('applies configured pools to local builds', () => {
    const config = makeConfig()
    config.sisoLimits = { local: 3, preproc: 2 }
    expect(limits(config)).toEqual({ local: '3', preproc: '2' })
  })

  it('keeps configured pool overrides and tighter inherited limits', () => {
    const config = makeConfig(false, ['--ninja=j:6'])
    config.sisoLimits = { local: 3, preproc: 4 }
    process.env.SISO_LIMITS = 'local=2,preproc=8'
    expect(limits(config)).toEqual({ local: '2', preproc: '4' })
  })

  it('does not apply RBE-only environment changes to local builds', () => {
    process.env.NINJA_CORE_LIMIT = '1000'
    process.env.NINJA_CORE_MULTIPLIER = '40'
    const env = makeConfig(false, ['--ninja=j:6']).defaultOptions.env
    expect(env.NINJA_CORE_LIMIT).toBe('1000')
    expect(env.NINJA_CORE_MULTIPLIER).toBe('40')
    expect(env.RBE_exec_timeout).toBeUndefined()
    expect(env.RBE_reclient_timeout).toBeUndefined()
  })

  it('preserves other Ninja options alongside the jobs workaround', () => {
    const config = makeConfig(false, ['--ninja=j:6', '--ninja=k:0'])
    expect(config.extraNinjaOpts).toContain('-k')
    expect(config.extraNinjaOpts).toContain('0')
    expect(config.extraNinjaOpts).not.toContain('-j')
    expect(limits(config).local).toBe('6')
  })

  it('preserves the remote jobs override', () => {
    expect(limits(makeConfig(true, ['--ninja=j:6']))).toEqual({
      local: '6',
      remote: '6',
    })
  })

  it('preserves the remote hard cap without a jobs override', () => {
    const config = makeConfig(true)
    jest.spyOn(EnvConfig.prototype, 'getNumber').mockReturnValue(1440)
    const env = config.defaultOptions.env
    expect(env.SISO_LIMITS).toBe('remote=640')
    expect(env.NINJA_CORE_LIMIT).toBe('640')
    expect(env.NINJA_CORE_MULTIPLIER).toBe('20')
  })

  it('preserves tighter remote and local inherited limits', () => {
    process.env.SISO_LIMITS = 'local=2,remote=4,preproc=3'
    expect(limits(makeConfig(true, ['--ninja=j:6']))).toEqual({
      local: '2',
      remote: '4',
      preproc: '3',
    })
  })

  it('preserves configured per-pool overrides for remote builds', () => {
    const config = makeConfig(true, ['--ninja=j:6'])
    config.sisoLimits = { local: 3, remote: 7 }
    expect(limits(config)).toEqual({ local: '3', remote: '7' })
  })

  it('preserves remote offline resource settings', () => {
    const config = makeConfig(true, ['--ninja=j:6', '--offline'])
    expect(config.defaultOptions.env.RBE_local_resource_fraction).toBe('1.0')
    expect(limits(config)).toEqual({ local: '6', remote: '6' })
  })
})
