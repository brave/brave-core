/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import path from 'node:path'

import configureNullability from './androidStudioNullability.js'
import config from './config.ts'
import util from './util.js'
import * as Log from './log.ts'

// Mirrors the --project-dir handling of generate_gradle.py, which defaults to
// $CHROMIUM_OUTPUT_DIR/gradle.
const getProjectDir = (passthroughArgs) => {
  let projectDir = path.join('$CHROMIUM_OUTPUT_DIR', 'gradle')
  const flagIndex = passthroughArgs.indexOf('--project-dir')
  const inlineArg = passthroughArgs.find((arg) =>
    arg.startsWith('--project-dir='),
  )
  if (flagIndex !== -1 && flagIndex + 1 < passthroughArgs.length) {
    projectDir = passthroughArgs[flagIndex + 1]
  } else if (inlineArg) {
    projectDir = inlineArg.substring('--project-dir='.length)
  }
  return path.resolve(
    projectDir.replace('$CHROMIUM_OUTPUT_DIR', config.outputDir),
  )
}

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

    // Make Android Studio understand Chromium's nullness annotations, so it
    // does not flag @NullMarked code for missing @NonNull annotations.
    const status = configureNullability(getProjectDir(passthroughArgs))
    Log.status(`IDE nullability: ${status}`)
  })
}

export default genGradle
