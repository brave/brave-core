// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const config = require('../lib/config')
const util = require('../lib/util')

const gnCheck = (buildConfig = config.defaultBuildConfig, options = {}) => {
  config.buildConfig = buildConfig
  config.update(options)
  // util.run('gn', ['check', config.outputDir], config.defaultOptions)
  util.run(
    'python3',
    [
      'buildtools/checkdeps/checkdeps.py',
      'brave',
      '--extra-repos=brave',
      '--no-resolve-dotdot',
    ],
    config.defaultOptions
  )
}

module.exports = gnCheck
