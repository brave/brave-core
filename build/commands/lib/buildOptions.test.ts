// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Command } from 'commander'
import { supportQuiet } from './buildOptions.ts'
import config from './config.ts'
import * as Log from './log.ts'

jest.mock('./config.ts', () => ({
  __esModule: true,
  default: { quiet: undefined },
}))

describe('supportQuiet', () => {
  let setQuiet: jest.SpiedFunction<typeof Log.setQuiet>

  beforeEach(() => {
    setQuiet = jest.spyOn(Log, 'setQuiet').mockImplementation(() => {})
  })

  afterEach(() => {
    setQuiet.mockRestore()
    config.quiet = undefined
  })

  async function run(...args: string[]) {
    const command = supportQuiet(new Command('sync').action(() => {}))
    await command.parseAsync(args, { from: 'user' })
    return setQuiet.mock.calls[0]![0]
  }

  it('leaves the default to setQuiet without a flag or .env value', async () => {
    expect(await run()).toBeUndefined()
  })

  it('uses the .env value without a flag', async () => {
    config.quiet = false
    expect(await run()).toBe(false)
  })

  it('lets a flag take precedence over the .env value', async () => {
    config.quiet = false
    expect(await run('--quiet')).toBe(true)
    setQuiet.mockClear()
    config.quiet = true
    expect(await run('--no-quiet')).toBe(false)
    setQuiet.mockClear()
    expect(await run('--verbose')).toBe(false)
  })
})
