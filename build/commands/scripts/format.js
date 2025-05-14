// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const fs = require('fs-extra')
const prettier = require('prettier')
const program = require('commander')
const { spawnSync } = require('child_process')

const config = require('../lib/config')
const util = require('../lib/util')
const Log = require('../lib/logging')

program
  .description(
    'Format changed code with git cl format and prettier.\n' +
      'The changed code is the difference between the current files on disk\n' +
      'and the base branch (origin/master by default).\n' +
      'This script is also called as part of presubmit checks.'
  )
  .option('--base <base branch>', 'set the destination branch for the PR')
  .option(
    '--full',
    'format all lines in changed files instead of only the changed lines'
  )
  .option(
    '--presubmit',
    'filter out formatters that have dedicated presubmit checks.' +
      'Used when running the script from a presubmit'
  )
  .option(
    '--dry-run',
    "dry run, don't format files, just check if they are formatted"
  )
  .parse(process.argv)

// A function that formats the code in the current diff with base branch.
// It uses git cl format and prettier, then aggregates the results.
const runFormat = async (options = {}) => {
  if (!options.base) {
    options.base = 'origin/master'
  }
  const skipLogging = options.presubmit

  let cmdOptions = config.defaultOptions
  cmdOptions.cwd = config.braveCoreDir
  cmdOptions = util.mergeWithDefault(cmdOptions)
  const args = ['cl', 'format', '--upstream=' + options.base]

  args.push('--python')
  args.push('--no-rust-fmt')

  if (options.full) {
    args.push('--full')
  }

  if (options.presubmit) {
    args.push('--presubmit')
  }

  if (options.dryRun) {
    args.push('--diff', '--dry-run')
  }

  let errors = []

  const clFormatResult = util.run('git', [...args], {
    ...cmdOptions,
    continueOnFail: true,
    skipLogging,
    stdio: 'pipe'
  })

  const clFormatOutput = [clFormatResult.stdout, clFormatResult.stderr].join(
    '\n'
  )

  if (clFormatResult.status === 2 && options.dryRun) {
    errors.push(clFormatOutput)
  } else if (clFormatResult.status !== 0) {
    Log.error('Fatal: git cl format failed:\n' + clFormatOutput)
    process.exit(clFormatResult.status)
  }

  const changedFiles = util.getChangedFiles(
    config.braveCoreDir,
    options.base,
    skipLogging
  )

  errors = errors.concat(await runPrettier(changedFiles, options.dryRun))

  if (options.dryRun && errors.length > 0) {
    console.error(errors.join('\n'))
    process.exit(2)
  }
}

const runPrettier = async (files, dryRun) => {
  const options = require(path.join(config.braveCoreDir, '.prettierrc'))
  const ignorePath = path.join(config.braveCoreDir, '.prettierignore')
  if (!fs.existsSync(ignorePath)) {
    throw new RuntimeError(`${ignorePath} file not found`)
  }

  const errors = []
  for (const file of files) {
    const fileInfo = await prettier.getFileInfo(file, {
      ignorePath: ignorePath,
      withNodeModules: false
    })

    if (fileInfo.ignored || !fileInfo.inferredParser) {
      continue
    }

    const content = await fs.readFile(file, { encoding: 'utf-8' })
    const formatted = await prettier.format(content, {
      ...options,
      parser: fileInfo.inferredParser
    })
    if (content !== formatted) {
      if (dryRun) {
        // Use `echo <formatted> | git diff --no-index <file> -` to show diff
        const diffResult = spawnSync('git', ['diff', '--no-index', file, '-'], {
          stdio: 'pipe',
          input: formatted,
        })
        const diffOutput = [diffResult.stdout, diffResult.stderr].join('\n')
        if (diffResult.status === 1) {
          // Differences were found
          errors.push(diffOutput)
        } else {
          errors.push(`Can't show diff for ${file}:\n ${diffOutput}`)
        }
      } else {
        await fs.writeFile(file, formatted)
      }
    }
  }

  return errors
}

runFormat(program.opts())
