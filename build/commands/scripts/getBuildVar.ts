// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { Argument, program } from 'commander'
import * as buildOptions from '../lib/buildOptions.ts'
import config, { type Config } from '../lib/config.ts'

const allowedBuildVars = {
  output_dir: 'outputDir',
} as const satisfies Record<string, keyof Config>

type BuildVar = keyof typeof allowedBuildVars

program
  .description('Print a build config variable for the given build config')
  .addArgument(
    new Argument('<variable>', 'build config variable to print').choices(
      Object.keys(allowedBuildVars) as BuildVar[],
    ),
  )
  .apply(buildOptions.supportBuildConfigArg)
  .apply(buildOptions.supportBuildDir)
  .apply(buildOptions.supportTargetConfig)
  .allowExcessArguments(true)
  .allowUnknownOption(true)
  .action(async (variable, buildConfig, options) => {
    config.buildConfig = buildConfig || config.defaultBuildConfig
    config.update(options)

    const value = config[allowedBuildVars[variable]]
    if (value === undefined) {
      console.error(`Build config variable "${variable}" is undefined`)
      process.exit(1)
    }

    console.log(config[allowedBuildVars[variable]])
  })
  .parseAsync()
