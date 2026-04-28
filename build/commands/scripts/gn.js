// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { program } from 'commander'
import config from '../lib/config.ts'
import util from '../lib/util.js'

program
  .description(
    [
      'Run a GN command in the output directory selected by a build config and target options.',
      'See https://gn.googlesource.com/gn/+/HEAD/docs/reference.md for GN commands reference.',
      '',
      'Examples:',
      '  gn check',
      '  gn check Static',
      '  gn check --target_os=android',
      '  gn check Static --target_os=android',
      '  gn refs Component //brave/browser',
    ].join('\n'),
  )
  .argument('<gn_command>', 'GN command to run.')
  .argument(
    '[build_config_or_gn_args...]',
    'optional build config followed by GN args; if omitted or the first value starts with "-", the default build config is used.',
  )
  .option('-C <build_dir>', 'override build dir; absolute or relative to out/')
  .option('--target_os <target_os>', 'target OS used to select the build dir')
  .option(
    '--target_arch <target_arch>',
    'target architecture used to select the build dir',
  )
  .option(
    '--target_environment <target_environment>',
    'target environment used to select the build dir (device, catalyst, simulator)',
  )
  .allowUnknownOption(true)
  .helpOption(false)
  .showHelpAfterError(true)
  .action(runGn)
  .parse()

async function runGn(gnCommand, args, options) {
  const gnArgs = [...args]
  const outputDirRequired = isOutputDirRequired(gnCommand)
  if (outputDirRequired) {
    config.buildConfig = getBuildConfigArg(gnArgs) || config.defaultBuildConfig
  }
  config.update(options)

  if (outputDirRequired) {
    gnArgs.unshift(config.outputDir)
  }

  util.run('gn', [gnCommand, ...gnArgs], config.defaultOptions)
}

function isOutputDirRequired(gnCommand) {
  return !['format', 'help'].includes(gnCommand)
}

function getBuildConfigArg(args) {
  if (args[0] && !args[0].startsWith('-')) {
    return args.shift()
  }
}
