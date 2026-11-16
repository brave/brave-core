// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import type { Command } from 'commander'
import rootDir from './rootDir.cjs'

type ConfigModule = typeof import('./config.ts')

interface LoadedProgram {
  program: Command
  config: ConfigModule['default']
  load: jest.SpiedFunction<ConfigModule['Config']['load']>
}

// A new `program`, with `Config.load()` returning a config read from no .env
// file.
async function loadProgram(): Promise<LoadedProgram> {
  let loaded: LoadedProgram | undefined
  await jest.isolateModulesAsync(async () => {
    const configModule: ConfigModule = await import('./config.ts')
    const { default: EnvConfig } = await import('./envConfig.ts')
    const { program } = await import('./program.ts')
    const load = jest
      .spyOn(configModule.Config, 'load')
      .mockImplementation(
        () =>
          new configModule.Config(
            new EnvConfig(path.join(rootDir, 'src', 'brave'), {}),
          ),
      )
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
})
