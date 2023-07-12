// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const util = require('../lib/util')
const config = require('../lib/config')
const path = require('path')


const runPerfTests = (passthroughArgs, perf_config, targets) => {
  targets = targets.replace(/brave:nightly:([^:]+):none:/, "$1:")
  if (process.platform === 'win32') {
    targets = '"' + targets + '"'
  }
  args = [
    path.join(
      config.braveCoreDir, 'tools', 'perf', 'run_dashboard_perftests.py'),
    '--config',
    perf_config,
    '--targets',
    targets,
    '--verbose',
  ]
  args.push(...passthroughArgs)
  console.log(args)
  util.run(
    'vpython3',
    args,
    config.defaultOptions)
}

module.exports = { runPerfTests }
