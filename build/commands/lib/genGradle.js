/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import config from './config.js'
import util from './util.js'
import * as Log from './log.ts'

const genGradle = (
  passthroughArgs,
  buildConfig = config.defaultBuildConfig,
  options,
) => {
  options.target_os = 'android'
  options.continueOnFail = false
  config.buildConfig = buildConfig
  config.update(options)
  Log.progressScope('Generating Gradle files', () => {
    let braveArgs = [
      'build/android/gradle/generate_gradle.py',
      '--output-directory',
      config.outputDir,
    ]

    const filteredArgs = passthroughArgs.filter(
      (arg) => !arg.includes('target_arch'),
    )
    braveArgs = braveArgs.concat(filteredArgs)

    util.run('python3', braveArgs, config.defaultOptions)
  })
}

export default genGradle
