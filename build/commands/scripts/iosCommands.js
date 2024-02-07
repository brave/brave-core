// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const fs = require('fs-extra')
const program = require('commander')
const path = require('path')
const config = require('../lib/config')
const util = require('../lib/util')

const createXCFrameworks = (buildConfig = config.defaultBuildConfig, options = {}) => {
  config.buildConfig = buildConfig
  config.targetOS = 'ios'
  config.update(options)

  const frameworks = ['BraveCore', 'MaterialComponents']
  frameworks.forEach((framework) => {
    const outputDir = path.join(config.outputDir, `${framework}.xcframework`)
    if (fs.existsSync(outputDir)) {
      fs.removeSync(outputDir)
    }
    const args = [
      '-create-xcframework',
      '-output', outputDir,
      '-framework', path.join(config.outputDir, `${framework}.framework`),
    ]
    // `-debug-symbols` must come after `-framework` or `-library`
    const symbolsDir = path.join(config.outputDir, `${framework}.dSYM`)
    if (fs.existsSync(symbolsDir)) {
      args.push('-debug-symbols', symbolsDir)
    }
    util.run('xcodebuild', args, config)
  })
}

const bootstrap = (options = {}) => {
  const bootstrapArgs = ['script/ios_bootstrap.py']
  if (options.force) {
    bootstrapArgs.push('--force')
  }
  util.run('vpython3', bootstrapArgs, config)
  if (options.open_xcodeproj) {
    const args = [
      path.join(config.srcDir, 'brave', 'ios', 'brave-ios', 'App', 'Client.xcodeproj')
    ]
    util.run('open', args)
  }
}

program
  .command('ios_create_xcframeworks')
  .option('--target_arch <target_arch>', 'target architecture', /^(host_cpu|x64|arm64|x86)$/i)
  .option('--target_environment <target_environment>', 'target environment (device, catalyst, simulator)', /^(device|catalyst|simulator)$/i)
  .arguments('[build_config]')
  .action(createXCFrameworks)

program
  .command('ios_bootstrap')
  .option('--open_xcodeproj', 'Open the Xcode project after bootstrapping')
  .option('--force', 'Always rewrite the symlink/directory entirely')
  .action(bootstrap)

program
  .parse(process.argv)
