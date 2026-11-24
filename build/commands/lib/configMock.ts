// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import { Config, setConfigInstance } from './config.ts'
import EnvConfig from './envConfig.ts'
import rootDir from './rootDir.cjs'

/**
 * Installs a new config as `config`, for tests. It has the checkout's
 * package.json, but takes its .env variables from `dotenv` rather than from a
 * .env file.
 *
 * @param dotenv - The .env variables the config sees
 * @returns The installed config
 */
export function mockConfig(dotenv: NodeJS.Dict<string> = {}): Config {
  const mock = new Config(
    new EnvConfig(path.join(rootDir, 'src', 'brave'), dotenv),
  )
  setConfigInstance(mock)
  return mock
}
