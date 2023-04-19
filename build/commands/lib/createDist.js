// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const config = require('../lib/config')
const util = require('../lib/util')
const path = require('path')
const fs = require('fs-extra')

const createDist = (buildConfig = config.defaultBuildConfig, options = {}) => {
  config.buildConfig = buildConfig
  config.update(options)
  util.touchOverriddenFiles()
  util.updateBranding()
  // On Android CI does two builds sequentially: for aab and for apk.
  // Symbols are uploaded after 2nd build, but we need to preserve the symbols
  // from the 1st build, so don't clean here dist folder; in anyway symbols zips
  // are overwritten and brave.breakpad.syms dir is cleared before generating
  // the symbols.
  if (config.targetOS !== 'android') {
    fs.removeSync(path.join(config.outputDir, 'dist'))
  }
  config.buildTarget = 'create_dist'
  util.generateNinjaFiles()
  util.buildTarget()
}

module.exports = createDist
