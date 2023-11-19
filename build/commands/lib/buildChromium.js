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

const outputArchive = `chromium_${config.braveVersion}_${config.targetArch}`

const chromiumConfigs = {
  'win': {
    buildTarget: 'mini_installer',
    processArtifacts: () => {
      fs.moveSync(
        path.join(config.outputDir, 'chrome.7z'),
        path.join(config.outputDir, `${outputArchive}.7z`))
    }
  },
  'linux': {
    buildTarget: 'installer',
    processArtifacts: () => {
      /* TODO */
    }
  },
  'mac': {
    buildTarget: 'installer',
    processArtifacts: () => {
      /* TODO */
    }
  },
  'android': {
    buildTarget: 'chrome_public_apk',
    processArtifacts: () => {
      fs.moveSync(
        path.join(config.outputDir, 'ChromePublic.apk'),
        path.join(config.outputDir, `${outputArchive}.apk`))
    }
  },
}

const buildChromium = (buildConfig = config.defaultBuildConfig, options = {}) => {
  config.buildConfig = buildConfig
  config.update(options)

  syncUtil.maybeInstallDepotTools()
  syncUtil.buildDefaultGClientConfig([config.getTargetOS()], [config.targetArch])

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

  const chromiumConfig = chromiumConfigs[config.getTargetOS()]
  config.buildTarget = chromiumConfig.buildTarget
  let args = {
    target_cpu: config.targetArch,
    target_os: config.getTargetOS(),
    enable_keystone_registration_framework: false,
    ignore_missing_widevine_signing_cert: true,
    is_chrome_branded: false,
    is_official_build: true,
    symbol_level: 1,
  }
  const buildArgsStr = util.buildArgsToString(args)

  util.run('gn', ['gen', config.outputDir, '--args="' + buildArgsStr + '"', config.extraGnGenOpts], config.defaultOptions)
  util.buildTarget()
  chromiumConfig.processArtifacts();
}

module.exports = buildChromium
