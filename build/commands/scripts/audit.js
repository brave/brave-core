/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const config = require('../lib/config')
const util = require('../lib/util')

let options = config.defaultOptions
options.continueOnFail = false
const outputDir = config.outputDir + '_audit'

util.updateBranding()
const args = config.buildArgs()
if (process.argv.includes('--audit_dev_deps')) {
  args.audit_dev_deps = true
}
util.runGnGen(outputDir, args, [], options)

let ninjaOpts = [
  '-C', outputDir, 'brave:audit_deps',
  ...config.extraNinjaOpts
]
util.run('autoninja', ninjaOpts, options)
