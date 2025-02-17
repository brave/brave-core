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
  .action((options) => {
    args = [path.join(config.srcDir, 'brave', 'build', 'rust', 'build_rust_toolchain_aux.py')]
    if (options.out_dir) {
      args.push('--out-dir=' + options.out_dir)
    }
    util.run('python3', args, config.defaultOptions)
  })

program.parse(process.argv)
