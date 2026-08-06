// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import { program } from 'commander'
import { writeTsConfig } from './ts-config.ts'
import { generatePathMap } from './path-map.ts'

program
  .requiredOption('--root-gen-dir <path>', 'path to the build gen directory')
  .requiredOption('--name <name>', 'name of the generated tsconfig file')
  .requiredOption(
    '--extends-from <path>',
    'full path of the tsconfig to extend',
  )
  .action(async (options) => {
    const rootGenDir = path.resolve(options.rootGenDir)
    const extendsFrom = path.resolve(options.extendsFrom)
    const pathMap = generatePathMap(rootGenDir)
    await writeTsConfig(pathMap, rootGenDir, options.name, extendsFrom)
  })
  .parseAsync()
