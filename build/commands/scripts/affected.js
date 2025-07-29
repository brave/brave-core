// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const { listAffectedTests } = require('../lib/listAffectedTests')
const config = require('../lib/config')

module.exports = (program) =>
  program
    .command('list_affected_tests')
    .description('prints the tests that need to be run given the file changes')
    .option('-C [build_dir]', 'build config (out/Debug, out/Release)')
    .option('--target_arch [target_arch]', 'target architecture')
    .option('--suite [suite]', 'filter by a test suite group')
    .option(
      '--filters [filters...]',
      'filter tests by a gn target pattern. eg. //brave/*. '
        + 'Make sure you put the list of filters in quotes. defaults to //*',
      (x) => x.split(' '),
    )
    .option(
      '--since_commit [targetCommit]',
      'use this commit as reference for change detection',
    )
    .option(
      '--files [files...]',
      'add additional files to be considered as modified analyze.'
        + 'You need to quote the list. '
        + 'You can use this to test what impact a file change would have',
      (x) => x.split(' '),
    )
    .action(async (args) => {
      config.update(args)
      const result = await listAffectedTests({ ...args })
      console.log(result.join(' ').trim())
    })
