// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const util = require('../lib/util')
const config = require('../lib/config')
const path = require('path')
const fs = require('fs')

const runPerfTests = (
  passthroughArgs,
  perfConfig,
  targetBuildConfig,
  options,
) => {
  args = [
    path.join(config.braveCoreDir, 'tools', 'perf', 'run_perftests.py'),
    perfConfig,
  ]

  if (perfConfig === 'smoke') {
    // Launch python unit tests before running perf tests.
    const perfDir = path.join(config.braveCoreDir, 'tools', 'perf')
    util.run('vpython3', ['-m', 'unittest', 'discover', '-p', '*_test.py'], {
      ...config.defaultOptions,
      cwd: perfDir,
    })
  }

  if (['Static', 'Release', 'Component'].includes(targetBuildConfig)) {
    config.buildConfig = targetBuildConfig
    config.update(options)

    binaryPath = path.join(config.outputDir, 'brave')
    if (process.platform === 'win32') {
      binaryPath += '.exe'
    } else if (process.platform === 'darwin') {
      const helperPath = binaryPath + '_helper'
      if (!fs.existsSync(helperPath)) {
        console.log(`${helperPath} not found, run build first`)
        process.exit(1)
      }

      binaryPath = fs.readFileSync(helperPath).toString().trim()

      // Convert "\ " to " ":
      binaryPath = binaryPath.replace(/\\ /g, ' ')
      if (!fs.existsSync(binaryPath)) {
        console.log(`${binaryPath} not found, run build first`)
        process.exit(1)
      }
    }
    const braveCoreCommit = util.runGit(
      config.braveCoreDir,
      ['rev-parse', 'HEAD'],
      true,
    )
    targetBuildConfig = `${braveCoreCommit}:${binaryPath}`
  }

  if (targetBuildConfig !== undefined) {
    args.push(targetBuildConfig)
  }

  args.push(...passthroughArgs)
  console.log(args)

  util.run('vpython3', args, config.defaultOptions)
}

module.exports = { runPerfTests }
