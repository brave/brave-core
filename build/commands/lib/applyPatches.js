// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const config = require('../lib/config')
const util = require('../lib/util')

const applyPatches = (
  buildConfig = config.defaultBuildConfig,
  options = {}
) => {
  async function RunCommand() {
    config.buildConfig = buildConfig
    config.update(options)
    await util.applyPatches(options.printPatchFailuresInJson)
  }

  RunCommand().catch((err) => {
    console.error(err)
    process.exit(1)
  })
}

module.exports = applyPatches
