/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const config = require('../lib/config')
const util = require('../lib/util')
const Log = require('./logging')

let options = config.defaultOptions
options.continueOnFail = false

const genGradle = (passthroughArgs, build_dir) => {
        Log.progressStart('Generating Gradle files')
        args = [
            'build/android/gradle/generate_gradle.py',
            '--output-directory', 
            build_dir 
        ]
        args.push(... passthroughArgs)
        util.run('python3', args, options)
        Log.progressFinish('generating Gradle files')
}

module.exports = genGradle
