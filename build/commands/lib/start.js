// Copyright (c) 2017 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const fs = require('fs-extra')
const config = require('../lib/config')
const util = require('../lib/util')

const start = (
  passthroughArgs,
  buildConfig = config.defaultBuildConfig,
  options,
) => {
  config.buildConfig = buildConfig
  config.update(options)

  let braveArgs = ['--enable-logging', '--v=' + options.v]
  if (options.vmodule) {
    braveArgs.push('--vmodule=' + options.vmodule)
  }
  if (options.no_sandbox) {
    braveArgs.push('--no-sandbox')
  }
  if (options.disable_brave_extension) {
    braveArgs.push('--disable-brave-extension')
  }
  if (options.disable_brave_rewards_extension) {
    braveArgs.push('--disable-brave-rewards-extension')
  }
  if (options.disable_pdfjs_extension) {
    braveArgs.push('--disable-pdfjs-extension')
  }
  if (options.ui_mode) {
    braveArgs.push(`--ui-mode=${options.ui_mode}`)
  }
  if (!options.enable_brave_update) {
    // This only has meaning with MacOS and official build.
    braveArgs.push('--disable-brave-update')
  }
  if (options.disable_doh) {
    braveArgs.push('--disable-doh')
  }
  if (options.single_process) {
    braveArgs.push('--single-process')
  }
  if (options.show_component_extensions) {
    braveArgs.push('--show-component-extension-options')
  }
  if (options.rewards) {
    braveArgs.push(`--rewards=${options.rewards}`)
  }
  if (options.brave_ads_testing) {
    braveArgs.push('--brave-ads-testing')
  }
  if (options.brave_ads_debug) {
    braveArgs.push('--brave-ads-debug')
  }
  if (options.brave_ads_production) {
    braveArgs.push('--brave-ads-production')
  }
  if (options.brave_ads_staging) {
    braveArgs.push('--brave-ads-staging')
  }

  if (process.platform === 'darwin') {
    // Disable 'accept incoming network connections' and 'keychain access'
    // dialogs in MacOS. See //docs/mac_build_instructions.md for details.
    if (!options.use_real_keychain) {
      braveArgs.push('--use-mock-keychain')
    }
    if (!passthroughArgs.some((s) => s.startsWith('--disable-features'))) {
      braveArgs.push('--disable-features=DialMediaRouteProvider')
    }
  }

  braveArgs = braveArgs.concat(passthroughArgs)

  let userDataDir
  if (options.user_data_dir_name) {
    if (process.platform === 'darwin') {
      userDataDir = path.join(
        process.env.HOME,
        'Library',
        'Application\\ Support',
        'BraveSoftware',
        options.user_data_dir_name,
      )
    } else if (process.platform === 'win32') {
      userDataDir = path.join(
        process.env.LocalAppData,
        'BraveSoftware',
        options.user_data_dir_name,
      )
    } else {
      userDataDir = path.join(
        process.env.HOME,
        '.config',
        'BraveSoftware',
        options.user_data_dir_name,
      )
    }
    braveArgs.push('--user-data-dir=' + userDataDir)
  }

  let cmdOptions = {
    stdio: 'inherit',
    timeout: undefined,
    continueOnFail: false,
    shell: process.platform === 'darwin',
    killSignal: 'SIGTERM',
  }

  let outputPath = options.output_path
  if (!outputPath) {
    outputPath = path.join(config.outputDir, 'brave')
    if (process.platform === 'win32') {
      outputPath = outputPath + '.exe'
    } else if (process.platform === 'darwin') {
      outputPath = fs
        .readFileSync(outputPath + '_helper')
        .toString()
        .trim()
    }
  }
  util.run(outputPath, braveArgs, cmdOptions)
}

module.exports = start
