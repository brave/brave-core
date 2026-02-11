// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

require('../lib/checkEnvironment')

const program = require('commander')
const config = require('../lib/config')
const util = require('../lib/util')

program
  .option('--base <base branch>', 'set the destination branch for the PR')
  .option(
    '--all [mode]',
    'run presubmit on all files: brave (default, specific checks) '
      + 'or chromium (all checks)',
  )
  .option(
    '--files <file list>',
    'semicolon-separated list files to run presubmit on',
  )
  .option(
    '--verbose [arg]',
    'pass --verbose 2 for more debugging info',
    (val) => (val === undefined ? 1 : parseInt(val, 10)),
  )
  .option('--fix', 'try to fix found issues automatically')
  .option('--json <output>', 'an output file for a JSON report')
  .action(runPresubmit)

function runPresubmit(options) {
  if (!options.base) {
    options.base = 'origin/master'
  }
  // Temporary cleanup call, should be removed when everyone will remove
  // 'gerrit.host' from their brave checkout.
  util.runGit(
    config.braveCoreDir,
    ['config', '--unset-all', 'gerrit.host'],
    true,
  )
  const cmdOptions = util.mergeWithDefault({ cwd: config.braveCoreDir })

  // --upload mode is similar to `git cl upload`. Non-upload mode covers less
  // checks.
  const args = ['cl', 'presubmit', options.base, '--force', '--upload']

  if (options.all) {
    if (options.all === 'brave' || options.all === true /* default value */) {
      cmdOptions.env.PRESUBMIT_ALL_BRAVE = '1'
    } else if (options.all === 'chromium') {
      args.push('--all')
    } else {
      throw new Error(`Invalid all mode: ${options.all}`)
    }
  }
  if (options.files) {
    args.push('--files', `"${options.files}"`)
  }
  if (options.verbose) {
    args.push(...Array(options.verbose).fill('--verbose'))
  }
  if (options.json) {
    args.push('-j')
    args.push(options.json)
  }

  if (options.fix) {
    cmdOptions.env.PRESUBMIT_FIX = '1'
  }
  util.run('git', args, cmdOptions)
}

program.parse(process.argv)
