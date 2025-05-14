/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const config = require('../lib/config')
const path = require('path')
const program = require('commander')
const util = require('../lib/util')

program
  .option('--out_dir <out_dir>', 'Path to put the build artifacts.')
  .option('--tag <tag>', '(optional) A Chromium tag to be checked out for generating this toolchain.')
  .action((options) => {
    if (options.tag) {
      util.runGit(config.srcDir, ['fetch', 'https://chromium.googlesource.com/chromium/src', 'tag', options.tag])
      util.runGit(config.srcDir, ['reset', '--hard', options.tag])

      // Setting this env variable to prevent downloading the toolchain using
      // the new tag's revision, as the intent is to generate a new toolchain.
      process.env.SKIP_DOWNLOAD_RUST_TOOLCHAIN_AUX = '1'
      util.runGClient(['sync', '--reset', '--upstream', '--revision', 'src@refs/tags/' + options.tag])
    }
    args = [path.join(config.srcDir, 'brave', 'build', 'rust', 'build_rust_toolchain_aux.py')]
    if (options.out_dir) {
      args.push('--out-dir=' + options.out_dir)
    }
    util.run('python3', args, config.defaultOptions)
  })

program.parse(process.argv)
