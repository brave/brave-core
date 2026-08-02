// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { Argument, program } from 'commander'
import { createBuildConfigArgument } from '../lib/commandsUtils.ts'
import path from 'node:path'
import config from '../lib/config.ts'
import util from '../lib/util.js'
import { storybookCommand } from '../../storybook/storybook_command.ts'

program
  .description(
    'Build Storybook generated deps, then run Storybook (dev or static build)',
  )
  .addArgument(
    new Argument('<mode>', 'Storybook mode').choices(['dev', 'build']),
  )
  .addArgument(createBuildConfigArgument())
  .option('-C <build_dir>', 'build directory, relative to out/ or absolute')
  .option('--target_arch <target_arch>', 'target architecture')
  .option('--target_os <target_os>', 'target OS')
  .option('-B, --build_deps', 'build Storybook GN deps')
  .allowExcessArguments(true)
  .allowUnknownOption(true)
  .action(async (mode, buildConfig, options) => {
    const buildConfigProvided =
      buildConfig !== undefined
      || options.C !== undefined
      || options.target_arch !== undefined
      || options.target_os !== undefined

    if (!buildConfigProvided) {
      // If no build config was provided, use the build output path from
      // guessConfig.js to set the build directory. This is a fallback for CI to
      // run storybook without specifying a build config, but still use the
      // right build output path.
      await import('../lib/guessConfig.js').then(({ outputPath }) => {
        console.log(`Using build output path from guessConfig: ${outputPath}`)
        options.C = outputPath
      })
    } else {
      config.buildConfig = buildConfig || config.defaultBuildConfig
    }

    config.update(options)

    if (mode === 'build' || options.build_deps) {
      config.buildTargets = ['brave/build/storybook:storybook_deps']
      await util.buildTargets(config.buildTargets, config.defaultOptions)
    } else {
      console.log('Skipping build of Storybook GN deps, pass -B to build them')
    }

    const storybookArgs = [
      mode,
      `--root-gen-dir=${path.join(config.outputDir, 'gen')}`,
    ]

    if (mode === 'build') {
      // If no build config was provided, use brave/.storybook-out directory.
      // This is a fallback for CI to generate storybook at the CI-expected
      // output path.
      const outputDir = buildConfigProvided
        ? path.join(config.outputDir, 'storybook')
        : path.join(config.braveCoreDir, '.storybook-out')
      storybookArgs.push(`--output-dir=${outputDir}`)
    }

    if (mode === 'dev') {
      // Pass through any additional arguments in dev mode.
      storybookArgs.push(...program.args)
    }

    await storybookCommand.parseAsync(storybookArgs, { from: 'user' })
  })
  .parseAsync()
