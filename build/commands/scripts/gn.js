// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
require('../lib/checkEnvironment')

const program = require('commander')
const config = require('../lib/config')
const util = require('../lib/util')

program
  .arguments('<gn_command> [build_config] [gn_args...]')
  .option('-C <build_dir>', 'absolute or relative to out/ build dir')
  .option('--target_os <target_os>', 'target OS')
  .option('--target_arch <target_arch>', 'target architecture')
  .option(
    '--target_environment <target_environment>',
    'target environment (device, catalyst, simulator)',
  )
  .allowUnknownOption(true)
  .action(runGn)
  .parse(process.argv)

async function runGn(gnCommand, buildConfig, gnArgs, options) {
  config.buildConfig = buildConfig || config.defaultBuildConfig
  config.update(options)

  util.run(
    'gn',
    [gnCommand, config.outputDir, ...gnArgs, ...getUnknownOptions(program)],
    config.defaultOptions,
  )
}

function* getUnknownOptions(command) {
  const opts = command.opts()
  for (const option of command.parseOptions(process.argv).unknown) {
    // Filter out actually known options passed as --option=value.
    const parsedOption = option.match(/--(.+)=.*?/)
    if (parsedOption && parsedOption[1] in opts) {
      continue
    }
    yield option
  }
}
