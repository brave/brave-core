// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import { Config } from './config.ts'
import EnvConfig from './envConfig.ts'
import rootDir from './rootDir.cjs'

// The real EnvConfig, recording what each instance is constructed with. Not a
// jest.fn(), whose implementation the `resetMocks` setting clears.
jest.mock('./envConfig.ts', () => {
  const ActualEnvConfig = jest.requireActual('./envConfig.ts').default
  class RecordingEnvConfig extends ActualEnvConfig {
    static constructedWith: unknown[][] = []

    constructor(...args: unknown[]) {
      super(...args)
      RecordingEnvConfig.constructedWith.push(args)
    }
  }
  return { __esModule: true, default: RecordingEnvConfig }
})

const { constructedWith } = EnvConfig as unknown as {
  constructedWith: unknown[][]
}

jest.mock('./log.ts', () => ({
  warn: jest.fn(),
  error: jest.fn(),
}))

describe('Config.useBuilder', () => {
  it('reads .env only for the keys --builder allows', () => {
    const config = new Config()
    config.useBuilder()
    expect(constructedWith.at(-1)).toEqual([
      path.join(rootDir, 'src', 'brave'),
      { dotenvKeys: ['rbe_service', 'siso_cache_dir', 'use_remoteexec'] },
    ])
  })

  it('reloads the values read before', () => {
    const config = new Config()
    // Not an allowed .env key, so it is back to its default.
    config.gclientVerbose = true
    config.useBuilder()
    expect(config.gclientVerbose).toBe(false)
  })
})
