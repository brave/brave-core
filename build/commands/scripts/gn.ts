// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { program } from 'commander'
import config from '../lib/config.ts'
import util from '../lib/util.js'

import * as build from '../lib/build.ts'

const root = program
  .description(
    [
      'Run a GN command in the output directory.',
      'See https://gn.googlesource.com/gn/+/HEAD/docs/reference.md for GN commands reference.',
      '',
      'Examples:',
      '  gn check',
      '  gn check static',
      '  gn refs android //brave/browser',
    ].join('\n'),
  )
  .argument('<gn_command>', 'GN command to run.')
  .argument(
    '[output_dir_or_gn_args...]',
    'optional output directory followed by GN args; if omitted or the first value starts with "-", the default output directory is used.',
  )
  .allowUnknownOption(true)
  .helpOption(false)
  .showHelpAfterError(true)

root.action((gnCommand, args, options) => {
  const gnArgs = [...args]
  const outputDirRequired = isOutputDirRequired(gnCommand)
  if (outputDirRequired) {
    const outputDir = getOutputDirArg(gnArgs)
    if (outputDir) {
      config.outputDir = outputDir
    } else {
      config.outputDir = 'Default'
    }
  }
  config.update(options)

  if (outputDirRequired) {
    gnArgs.unshift(config.outputDir)
  }

  util.run('gn', [gnCommand, ...gnArgs], config.defaultOptions)
})

build
  .addGnGenOptions(
    build.addGnArgsOptions(
      root
        .command('gen')
        .argument('[output_dir...]', 'output directory name')
        .option(
          '--config <build_config>',
          'build configuration, if not specified, the output directory is used',
        ),
    ),
  )
  .allowUnknownOption(true)
  .action((args, options) => {
    config.setExplicitGnGen()
    const gnArgs = [...args]
    const outputDir = getOutputDirArg(gnArgs)
    if (outputDir) {
      config.outputDir = outputDir
    }
    if (options.config) {
      config.buildConfig = options.config
    } else if (outputDir) {
      config.buildConfig = outputDir
    }
    options.force_gn_gen = true
    config.update(options)
    util.generateNinjaFiles(config.defaultOptions, gnArgs)
  })

root.parse()

function isOutputDirRequired(gnCommand) {
  return !['format', 'help'].includes(gnCommand)
}

function getOutputDirArg(args) {
  if (args[0] && !args[0].startsWith('-')) {
    return args.shift()
  }
}
