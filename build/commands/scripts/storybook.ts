// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { spawnSync } from 'node:child_process'
import fs from 'node:fs'
import path from 'node:path'
import process from 'node:process'
import { Option, program } from 'commander'
import { isCI } from '../lib/ciDetect.ts'
import * as buildOptions from '../lib/buildOptions.ts'
import config from '../lib/config.ts'
import util from '../lib/util.js'
import { getPassthroughArgs } from '../lib/commandsUtils.ts'

program
  .description(
    'Build Storybook generated deps, then run Storybook (dev or static build)',
  )
  .apply(buildOptions.supportBuildConfigArg)
  .apply(buildOptions.supportBuildDir)
  .apply(buildOptions.supportTargetConfig)
  .addOption(
    new Option('--command <command>', 'Storybook command')
      .choices(['build', 'dev'])
      .makeOptionMandatory(),
  )
  .option('-B, --build_deps', 'build Storybook GN deps')
  .allowExcessArguments(true)
  .allowUnknownOption(true)
  .action(async (buildConfig, options) => {
    if (buildConfig) {
      config.buildConfig = buildConfig
    }

    const buildConfigProvided =
      buildConfig !== undefined
      || options.C !== undefined
      || options.target_arch !== undefined
      || options.target_os !== undefined

    if (!buildConfigProvided && !isCI) {
      // If no build config was provided, use the build output path from
      // guessConfig.js to set the build directory.
      await import('../lib/guessConfig.js').then(({ outputPath }) => {
        options.C = outputPath
      })
    }

    config.update(options)

    if (options.command === 'build' || options.build_deps) {
      config.buildTargets = ['brave/build/storybook:storybook_deps']
      await util.buildTargets(config.buildTargets, config.defaultOptions)
    }

    const rootGenDir = path.join(config.outputDir, 'gen')
    if (!fs.existsSync(rootGenDir)) {
      program.error(
        `Failed to find build output 'gen' folder at '${rootGenDir}'. `
          + 'Have you run a brave-core build yet with the specified '
          + '(or default) configuration?',
      )
    }

    const storybookCli = path.join(
      config.braveCoreDir,
      'node_modules',
      'storybook',
      'bin',
      'index.cjs',
    )
    const storybookConfigDir = path.join(
      config.braveCoreDir,
      'build',
      'storybook',
    )

    const nodeArgs = [
      // Fix occasional webpack build failures by increasing the V8's old memory
      // section size.
      '--max-old-space-size=8192',
      storybookCli,
      options.command,
      '-c',
      storybookConfigDir,
    ]

    switch (options.command) {
      case 'build':
        nodeArgs.push('-o', path.join(config.outputDir, 'storybook'))
        break
      case 'dev':
        // Pass through any additional arguments in dev mode.
        nodeArgs.push(...getPassthroughArgs(program))
        break
    }

    const result = spawnSync(process.execPath, nodeArgs, {
      env: {
        ...process.env,
        ROOT_GEN_DIR: rootGenDir,
      },
      stdio: 'inherit',
      cwd: config.braveCoreDir,
    })

    if (result.status !== 0) {
      process.exit(result.status ?? 1)
    }
  })
  .parseAsync()
