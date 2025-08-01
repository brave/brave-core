// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const util = require('../lib/util')
const config = require('../lib/config')
const path = require('path')
const fs = require('fs')

const runPerfTests = (passthroughArgs, perfConfig, targetBuildConfig) => {
  args = [
    path.join(config.braveCoreDir, 'tools', 'perf', 'run_perftests.py'),
    perfConfig,
  ]

  if (['Static', 'Release', 'Component'].includes(targetBuildConfig)) {
    config.buildConfig = targetBuildConfig
    config.update({})

    binaryPath = path.join(config.outputDir, 'brave')
    if (process.platform === 'win32') {
      binaryPath += '.exe'
    } else if (process.platform === 'darwin') {
      binaryPath = fs
        .readFileSync(binaryPath + '_helper')
        .toString()
        .trim()

      // Convert "\ " to " ":
      binaryPath = binaryPath.replace(/\\ /g, ' ')
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

  let cmdOptions = {
    ...config.defaultOptions,
    shell: false,
  }
  util.run('vpython3', args, cmdOptions)
}

module.exports = { runPerfTests }
