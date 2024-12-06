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
const depotTools = require('./depotTools')
const syncUtil = require('./syncUtils')
const Log = require('./logging')

// Use the same filename as for Brave archive.
const getOutputFilename = () => {
  const platform = (() => {
    if (config.getTargetOS() === 'win')
      return 'win32'
    if (config.getTargetOS() === 'mac')
      return 'darwin'
    return config.getTargetOS()
  })()
  return `chromium-${config.chromeVersion}-${platform}-${config.targetArch}`
}

const chromiumConfigs = {
  'win': {
    buildTargets: ['mini_installer'],
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
    buildTargets: ['chrome/installer/linux:stable_deb'],
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
    buildTargets: ['chrome'],
    extraHooks: () => {
      Log.progressScope('download_hermetic_xcode', () => {
        util.run('vpython3',
          [
            path.join(config.braveCoreDir,
              'build', 'mac', 'download_hermetic_xcode.py'),
          ],
          config.defaultOptions)
      })
    },
    processArtifacts: () => {
      util.run('zip',
        ['-r', '-y', `${getOutputFilename()}.zip`, 'Chromium.app'],
        { cwd: config.outputDir }
      )
    }
  },
  'android': {
    buildTargets: ['monochrome_64_public_apk'],
    processArtifacts: () => {
      fs.moveSync(
        path.join(config.outputDir, 'apks', 'MonochromePublic64.apk'),
        path.join(config.outputDir, `${getOutputFilename()}.apk`))
    }
  },
}

// A function to make gn args to build a release Chromium build.
// There is two primarily sources:
// 1. Chromium perf builds: tools/mb/mb_config_expectations/chromium.perf.json
// 2. Brave Release build configuration
function getChromiumGnArgs() {
  const targetOs = config.getTargetOS()
  const targetArch = config.targetArch
  const args = {
    target_cpu: targetArch,
    target_os: targetOs,
    is_official_build: true,
    ffmpeg_branding: 'Chrome',
    enable_widevine: true,
    ignore_missing_widevine_signing_cert: true,
    skip_secondary_abi_for_cq: true,
    ...config.extraGnArgs,
  }

  if (targetOs === 'android') {
    args.debuggable_apks = false
  } else {
    args.enable_hangout_services_extension = false
    args.enable_nacl = false
  }

  if (targetOs === 'mac') {
    args.use_system_xcode = true
  }

  return args
}

function buildChromiumRelease(buildOptions = {}) {
  if (!config.isCI && !buildOptions.force) {
    console.error(
      'Warning: the command resets all changes in src/ folder.\n' +
      'src/brave stays untouched. Pass --force to continue.')
    return 1
  }
  config.buildConfig = 'Release'
  config.isChromium = true
  config.update(buildOptions)

  const chromiumConfig = chromiumConfigs[config.getTargetOS()]
  if (chromiumConfig == undefined)
    throw Error(`${config.getTargetOS()} is unsupported`)

  depotTools.installDepotTools()
  syncUtil.buildDefaultGClientConfig(
    [config.getTargetOS()], [config.targetArch], true)

  util.runGit(config.srcDir, ['clean', '-f', '-d'])


  Log.progressScope('gclient sync', () => {
    syncUtil.syncChromium({ force: true, sync_chromium: true })
  })

  Log.progressScope('gclient runhooks', () => {
    util.runGClient(['runhooks'])
  })

  if (chromiumConfig.extraHooks != undefined) {
    chromiumConfig.extraHooks()
  }

  // A workaround for
  // https://chromium-review.googlesource.com/c/chromium/src/+/6013664
  const v8CompileFix = '1e3bed631cff17487775e33626121bfd5f0e664e'
  util.runGit(config.srcDir, ['fetch', 'origin', v8CompileFix])
  util.runGit(config.srcDir, ['cherry-pick', 'FETCH_HEAD'])

  util.runGnGen(config.outputDir, getChromiumGnArgs())

  Log.progressScope(`remove recursive symlinks`, () => {
    // node_modules could have a symlink to src/brave. The recursive symlinks
    // break the logic of some chromium scripts and should be remove before
    // the build.
    const linkPath = path.join(config.braveCoreDir, 'node_modules',
      'brave-core')
    if (fs.existsSync(linkPath)) {
      fs.unlinkSync(linkPath);
    }
  })

  Log.progressScope(`ninja`, () => {
    const target = chromiumConfig.buildTargets
    const ninjaOpts = [
      '-C', config.outputDir, target.join(' '),
      ...config.extraNinjaOpts
    ]
    util.run('autoninja', ninjaOpts, config.defaultOptions)
  })

  Log.progressScope('make archive', () => {
    chromiumConfig.processArtifacts()
  })
}

module.exports = buildChromiumRelease
