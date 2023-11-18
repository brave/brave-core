// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const config = require('./config')
const util = require('./util')
const path = require('path')
const fs = require('fs-extra')
const syncUtil = require('./syncUtils')
const Log = require('./logging')

const buildChromium = (buildConfig = config.defaultBuildConfig, options = {}) => {
  syncUtil.maybeInstallDepotTools()
  syncUtil.buildDefaultGClientConfig(config.targetOS ? [config.targetOS]: null, [config.targetArch])

  util.runGit(config.srcDir, ['clean', '-f', '-d'])

  if (config.isCI) {
    program.delete_unused_deps = true
  }

  Log.progressScope('gclient sync', () => {
    syncUtil.syncChromium(true, true, false)
  })

  Log.progressScope('gclient runhooks', () => {
    util.runGClient(['runhooks'])
  })

  config.buildConfig = buildConfig
  config.update(options)
  config.buildTarget = 'chrome'
  let args = {
    enable_keystone_registration_framework: false,
    ignore_missing_widevine_signing_cert: true,
    is_chrome_branded: false,
    is_official_build: true,
    symbol_level: 1,
  }
  const buildArgsStr = util.buildArgsToString(args)

  util.run('gn', ['gen', config.outputDir, '--args="' + buildArgsStr + '"', config.extraGnGenOpts], config.defaultOptions)
  util.buildTarget()
}

module.exports = buildChromium
