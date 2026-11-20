// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { Builder } from './builderTypes.ts'
import { Config, setConfigInstance } from './config.ts'

/**
 * Installs a new config as `config`, for tests. It has the checkout's
 * package.json, but takes its .env variables from `dotenv` rather than from a
 * .env file.
 *
 * @param dotenv - The .env variables the config sees
 * @param builder - The `--builder` the config is for, if any
 * @returns The installed config
 */
export function mockConfig(
  dotenv: NodeJS.Dict<string> = {},
  builder?: Builder,
): Config {
  const mock = Config.load(builder, dotenv)
  setConfigInstance(mock)
  return mock
}
