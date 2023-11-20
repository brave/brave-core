// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// A script to build and pack a Chromium release build from scratch
// Reuses the same /src folder
// Designed to be used on CI, but should work locally too.
// The script includes syncing; there is no need to run npm run sync before.

const config = require('./config')
const util = require('./util')
const path = require('path')
const fs = require('fs-extra')
const syncUtil = require('./syncUtils')
const Log = require('./logging')

// Use the same filename as for Brave archive.
const getOutputFilename = () => {
  const getPlatfrom = () => {
    if (config.getTargetOS() === 'win')
      return 'win32';
    if (config.getTargetOS() === 'mac')
      return 'darwin';
    return config.getTargetOS()
  }
  return `chromium-${config.chromeVersion}-${getPlatfrom()}-${config.targetArch}`
}

const chromiumConfigs = {
  'win': {
    buildTarget: 'mini_installer',
    processArtifacts: () => {
      // Repack it to reduce the size and use .zip instead of .7z.
      input = path.join(config.outputDir, 'chrome.7z')
      output = path.join(config.outputDir, `${getOutputFilename()}.zip`)
      util.run('python3',
        [
          path.join(config.braveCoreDir, 'script', 'repack-archive.py'),
          `--input=${input}`,
          `--output=${output}`,
          '--target_dir=Chrome-bin',
        ],
        config.defaultOptions)
    }
  },
  'linux': {
    buildTarget: 'chrome/installer/linux:stable_deb',
    processArtifacts: () => {
      const deb_arch = (() => {
        if (config.targetArch === 'x64') return 'amd64'
        return config.targetArch
      })()
      fs.moveSync(
        path.join(config.outputDir,
          `chromium-browser-stable_${config.chromeVersion}-1_${deb_arch}.deb`),
        path.join(config.outputDir, `${getOutputFilename()}.deb`))
    }
  },
  'mac': {
    buildTarget: 'chrome',
    processArtifacts: () => {
      util.run('zip',
        ['-r', '-y', `${getOutputFilename()}.zip`, 'Chromium.app'],
        { cwd: config.outputDir }
      )
    }
  },
  'android': {
    buildTarget: 'chrome_public_apk',
    processArtifacts: () => {
      fs.moveSync(
        path.join(config.outputDir, 'apks', 'ChromePublic.apk'),
        path.join(config.outputDir, `${getOutputFilename()}.apk`))
    }
  },
}

const buildChromium = (buildConfig = config.defaultBuildConfig, options = {}) => {
  config.buildConfig = buildConfig
  config.update(options)

  const chromiumConfig = chromiumConfigs[config.getTargetOS()]
  if (chromiumConfig == undefined)
    throw Error(`${config.getTargetOS()} is unsupported`)

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

  config.buildTarget = chromiumConfig.buildTarget
  const args = {
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

  Log.progressScope('make archive', () => {
    chromiumConfig.processArtifacts()
  })
}

module.exports = buildChromium
