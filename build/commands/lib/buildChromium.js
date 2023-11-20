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
  const platform = (() => {
    if (config.getTargetOS() === 'win')
      return 'win32';
    if (config.getTargetOS() === 'mac')
      return 'darwin';
    return config.getTargetOS()
  })()
  return `chromium-${config.chromeVersion}-${platform}-${config.targetArch}`
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
      const debArch = (() => {
        if (config.targetArch === 'x64') return 'amd64'
        return config.targetArch
      })()
      fs.moveSync(
        path.join(config.outputDir,
          `chromium-browser-stable_${config.chromeVersion}-1_${debArch}.deb`),
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

// A function to make gn args to build a release Chromium build.
// There is two primarily sources:
// 1. Chromium perf builds: tools/mb/mb_config_expectations/chromium.perf.json
// 2. Brave Release build configuration
function getChromiumGnArgs() {
  const braveGnArgs = config.buildArgs()
  const chromiumGnArgs = {
    target_cpu: config.targetArch,
    target_os: config.getTargetOS(),
    is_official_build: true,
    symbol_level: 1,
    enable_keystone_registration_framework: false,
  }

  if (config.isAndroid)
    chromiumGnArgs.debuggable_apks = false

  const gnArgsToMirror = [
    'enable_hangout_services_extension',
    'enable_widevine',
    'ignore_missing_widevine_signing_cert',
    'enable_nacl',
    'ffmpeg_branding',
  ]

  if (braveGnArgs.use_system_xcode !== undefined) {
    chromiumGnArgs.use_system_xcode = braveGnArgs.use_system_xcode
  }

  for (const arg of gnArgsToMirror) {
    const braveArg = braveGnArgs[arg]
    if (braveArg == undefined)
      throw Error(`Gn arg ${arg} doesn't exist in config.buildArgs()`)
    chromiumGnArgs[arg] = braveArg
  }
  return chromiumGnArgs
}

function buildChromium(buildConfig = config.defaultBuildConfig, options = {}) {
  config.buildConfig = buildConfig
  config.update(options)
  config.outputDir = config.outputDir + '_chromium'

  const chromiumConfig = chromiumConfigs[config.getTargetOS()]
  if (chromiumConfig == undefined)
    throw Error(`${config.getTargetOS()} is unsupported`)

  syncUtil.maybeInstallDepotTools()
  syncUtil.buildDefaultGClientConfig(
    [config.getTargetOS()], [config.targetArch])

  util.runGit(config.srcDir, ['clean', '-f', '-d'])


  Log.progressScope('gclient sync', () => {
    syncUtil.syncChromium(true, true, false)
  })

  Log.progressScope('gclient runhooks', () => {
    util.runGClient(['runhooks'])
  })

  config.buildTarget = chromiumConfig.buildTarget
  const buildArgsStr = util.buildArgsToString(getChromiumGnArgs())
  util.run('gn', ['gen', config.outputDir,
    '--args="' + buildArgsStr + '"', config.extraGnGenOpts],
    config.defaultOptions)

  util.buildTarget()

  Log.progressScope('make archive', () => {
    chromiumConfig.processArtifacts()
  })
}

module.exports = buildChromium
