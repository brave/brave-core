// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import type { Command } from 'commander'
import { writeMockBuilder } from './builderMock.ts'
import rootDir from './rootDir.cjs'

type ConfigModule = typeof import('./config.ts')

interface LoadedProgram {
  program: Command
  config: ConfigModule['default']
  load: jest.SpiedFunction<ConfigModule['Config']['load']>
}

// `--builder` is refused on CI, where these tests also run.
jest.mock('./ciDetect.ts', () => ({ isCI: false, isTeamcity: false }))

jest.mock('./log.ts', () => ({
  ...jest.requireActual('./log.ts'),
  warn: jest.fn(),
}))

// Where `--builder` looks builders up, rather than infra/config. `var`, as
// the `./builder.ts` mock reads it when loaded, after this initialiser.
// eslint-disable-next-line no-var
var mockBuildersDir = fs.mkdtempSync(path.join(os.tmpdir(), 'builders-'))

jest.mock('./builder.ts', () => {
  const actual: typeof import('./builder.ts') =
    jest.requireActual('./builder.ts')
  return {
    ...actual,
    supportBuilder: (command: Command) =>
      actual.supportBuilder(command, { dir: mockBuildersDir }),
  }
})

const builderName = 'fake-builder'

beforeAll(() => {
  writeMockBuilder(mockBuildersDir, builderName)
})

afterAll(() => {
  fs.rmSync(mockBuildersDir, { recursive: true, force: true })
})

// A new `program`, with `Config.load()` reading no .env file.
async function loadProgram(): Promise<LoadedProgram> {
  let loaded: LoadedProgram | undefined
  await jest.isolateModulesAsync(async () => {
    const configModule: ConfigModule = await import('./config.ts')
    const { program } = await import('./program.ts')
    const { Config } = configModule
    const loadConfig = Config.load.bind(Config)
    const load = jest
      .spyOn(Config, 'load')
      .mockImplementation((builder) => loadConfig(builder, {}))
    loaded = { program, config: configModule.default, load }
  })
  return loaded!
}

describe('program', () => {
  it('installs the config before its action runs', async () => {
    const { program, config } = await loadProgram()
    let srcDir: string | undefined
    program.action(() => {
      srcDir = config.srcDir
    })
    await program.parseAsync([], { from: 'user' })
    expect(srcDir).toBe(path.join(rootDir, 'src'))
  })

  it('installs the config before the hooks added to it', async () => {
    const { program, config } = await loadProgram()
    let srcDir: string | undefined
    program
      .hook('preAction', () => {
        srcDir = config.srcDir
      })
      .action(() => {})
    await program.parseAsync([], { from: 'user' })
    expect(srcDir).toBe(path.join(rootDir, 'src'))
  })

  it('installs the config once for a subcommand', async () => {
    const { program, config, load } = await loadProgram()
    let srcDir: string | undefined
    program
      .command('sub')
      .hook('preAction', () => {})
      .action(() => {
        srcDir = config.srcDir
      })
    await program.parseAsync(['sub'], { from: 'user' })
    expect(srcDir).toBe(path.join(rootDir, 'src'))
    expect(load).toHaveBeenCalledTimes(1)
  })

  it('does not load the config without an action to run', async () => {
    const { program, load } = await loadProgram()
    program
      .exitOverride()
      .configureOutput({ writeOut: () => {} })
      .action(() => {})
    await expect(
      program.parseAsync(['--help'], { from: 'user' }),
    ).rejects.toThrow('(outputHelp)')
    expect(load).not.toHaveBeenCalled()
  })

  it('loads the config for the --builder given', async () => {
    const { program, config, load } = await loadProgram()
    let targetOS: string | undefined
    program.action(() => {
      targetOS = config.targetOS
    })
    await program.parseAsync(['--builder', builderName], { from: 'user' })
    expect(load).toHaveBeenCalledTimes(1)
    expect(load.mock.calls[0]![0]?.name).toBe(builderName)
    expect(config.isBuilderSet()).toBe(true)
    expect(targetOS).toBe('linux')
    expect(() => config.gclientVerbose).not.toThrow()
    expect(() => config.notary_user).toThrow('notary_user is read')
  })

  it('rejects an unknown --builder', async () => {
    const { program, load } = await loadProgram()
    program
      .exitOverride()
      .configureOutput({ writeErr: () => {} })
      .action(() => {})
    await expect(
      program.parseAsync(['--builder', 'nope'], { from: 'user' }),
    ).rejects.toThrow('unknown builder: nope')
    expect(load).not.toHaveBeenCalled()
  })
})
