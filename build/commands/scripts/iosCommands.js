// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { program, Option } from 'commander'
import fs from 'fs-extra'
import path from 'node:path'
import config from '../lib/config.ts'
import util from '../lib/util.js'
import { createBuildConfigArgument } from '../lib/commandsUtils.ts'

const bootstrap = (options = {}) => {
  const utilConfig = config.defaultOptions
  utilConfig.cwd = config.braveCoreDir
  const bootstrapArgs = ['script/ios_bootstrap.py']
  if (options.force) {
    bootstrapArgs.push('--force')
  }
  util.run('python3', bootstrapArgs, utilConfig)
  if (options.open_xcodeproj) {
    const args = [
      path.join(
        config.srcDir,
        'brave',
        'ios',
        'brave-ios',
        'App',
        'Client.xcodeproj',
      ),
    ]
    util.run('open', args)
  }
}

program
  .command('ios_bootstrap')
  .option('--open_xcodeproj', 'Open the Xcode project after bootstrapping')
  .option('--force', 'Always rewrite the symlink/directory entirely')
  .action(bootstrap)

program
  .command('ios_update_current_link')
  .description(
    'Updates the stable ios_current_link output directory symlink for upcoming Xcode builds',
  )
  .addOption(
    new Option('--target_arch <target_arch>', 'target architecture').choices([
      'arm64',
      'x64',
    ]),
  )
  .addOption(
    new Option(
      '--target_environment <target_environment>',
      'target environment',
    ).choices(['device', 'catalyst', 'simulator']),
  )
  .addArgument(createBuildConfigArgument())
  .action((buildConfig, options) => {
    config.buildConfig = buildConfig || config.defaultBuildConfig
    config.targetOS = 'ios'
    config.update(options)

    const currentLink = path.join(config.srcDir, 'out', 'ios_current_link')
    fs.removeSync(currentLink)
    fs.symlinkSync(config.outputDir, currentLink, 'junction')
  })

program.parse(process.argv)
