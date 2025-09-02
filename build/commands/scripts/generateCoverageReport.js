// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
const { glob, rm } = require('fs/promises')
const { writeJSON, mkdirp } = require('fs-extra')
const utils = require('../lib/util')
const config = require('../lib/config')

module.exports = (program) =>
  program
    .command('coverage_report')
    .description(
      [
        'generates a coverage report.',
        'Requires to build with --is_coverage and run test.',
        'HTML report will be in out/$BUILD_DIR/coverage/report/$NAME/dist',
      ].join('\n'),
    )
    .option('-C [build_dir]', 'build config (out/Debug, out/Release)')
    .option('--build_type [build_type]', 'build with coverage information ')
    .option('--target_arch [target_arch]', 'target architecture')
    .option('--target_os <target_os>', 'target OS')
    .option('--name [name]', 'name of the test report [defaults to coverage]', 'all')
    .option(
      '--clean',
      'delete recordings and reports. Either all or those that match --tests [testSuites]',
    )
    .option(
      '--tests [testSuites]',
      'comma seperated list of testsuites to consider. By default it parses all recordings',
    )
    .arguments('[build_config]')
    .action(async (buildConfig, args) => {
      config.buildConfig = buildConfig || config.buildConfig
      config.update(args)
      const dirsGlob = args.tests ? `{${args.tests}}/**` : '**'

      const out = `${config.outputDir}/coverage`
      const reportPath = `${out}/report/${args.name}`
      const profDataPath = `${reportPath}/coverage.profdata`
      const distPath = `${reportPath}/dist`


      const recordings = await Array.fromAsync(
        glob(`${out}/${dirsGlob}/*.profraw`),
      )

      if (args.clean) {
        await Promise.all(
          recordings
            .concat([reportPath])
            .map((path) => rm(path, { recursive: true, force: true })),
        )

        return
      }

      const allTestSuites = [
        ...new Set(
          recordings
            .map((x) => x.replace(out + '/', ''))
            .map((x) => x.split('/'))
            .filter((x) => x.length > 1)
            .map((x) => x[0]),
        ),
      ].map((x) => `${config.outputDir}/${x}`)

      // llvm-cov needs a .profdata (llvms .lcov equivalent) and
      // binaries that include the symbols that are instrumented to generate a report
      // Weirdly it appears that the json export is unaffected.
      // We probably want to use gcov / genhtml instead of llvm-cov
      // For now - if we can - only use brave_browser_tests binary
      // since it will have the most amount of symbols to generate the report.
      // It will still include coverage information of code paths of other test suites
      // as long as their symbols are present in the brave_browser_test binary
      const hasBBT = allTestSuites.find((x) =>
        x.includes('brave_browser_tests'),
      )
      const testSuites = hasBBT
        ? [`${config.outputDir}/brave_browser_tests`]
        : allTestSuites

      if (!recordings.length) {
          `glob ${out}/${dirsGlob}/*.profraw didn't yield any recordings!\n make sure you've built with --is_coverage and ran the appropriate tests`,
        )
        return
      }

      process.env.CWD = config.outputDir
      process.env.PATH = `${process.env.PATH}:${config.srcDir}/third_party/llvm-build/Release+Asserts/bin`
      // fetch coverage tools if not available
      await mkdirp(distPath)
      await utils.runAsync('python3', [
        `${config.srcDir}/tools/clang/scripts/update.py`,
        '--package=coverage_tools',
      ])
      await utils.runAsync('llvm-profdata', [
        'merge',
        '-sparse',
        '-o',
        profDataPath,
        ...recordings,
      ])
      await utils.runAsync('llvm-cov', [
        'show',
        `-compilation-dir=${config.outputDir}`,
        `-instr-profile=${profDataPath}`,
        '-format=html',
        `-output-dir=${distPath}`,
        ...testSuites,
      ])

      const output = await utils.runAsync(
        'llvm-cov',
        [
          'export',
          `-compilation-dir=${config.outputDir}`,
          `-instr-profile=${profDataPath}`,
          '--summary-only',
          ...testSuites,
        ],
        { stdio: 'pipe' },
      )

      try {
        const summary = JSON.parse(output)
        await writeJSON(`${distPath}/coverage.json`, summary)
        console.log(summary.data[0].totals)
      } catch (e) {
        console.error(e)
      }

      console.log(`\ncoverage reports written to ${out}/report`)
    })
