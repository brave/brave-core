// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const program = require('commander')
const path = require('path')
const config = require('../lib/config')
const util = require('../lib/util')

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
      path.join(config.srcDir, 'brave', 'ios', 'brave-ios', 'App', 'Client.xcodeproj')
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
  .parse(process.argv)
