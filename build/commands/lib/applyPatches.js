// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import config from './config.js'
import util from './util.js'
import * as Log from './log.ts'

const applyPatches = (
  buildConfig = config.defaultBuildConfig,
  options = {},
) => {
  async function RunCommand() {
    config.buildConfig = buildConfig
    config.update(options)
    await util.applyPatches(options.printPatchFailuresInJson)
  }

  RunCommand().catch((err) => {
    Log.fatal(err)
  })
}

export default applyPatches
