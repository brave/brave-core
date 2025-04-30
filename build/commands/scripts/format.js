// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const fs = require('fs-extra')
const prettier = require('prettier')
const program = require('commander')

const config = require('../lib/config')
const util = require('../lib/util')

const mergeWithDefault = (options) => {
  return Object.assign({}, config.defaultOptions, options)
}

// A function that formats the code in the current diff with base branch.
// It uses git cl format and prettier, then aggregates the results.
const runFormat = async (options = {}) => {
  if (!options.base) {
    options.base = 'origin/master'
  }
  let cmdOptions = config.defaultOptions
  cmdOptions.cwd = config.braveCoreDir
  cmdOptions = mergeWithDefault(cmdOptions)
  cmd = 'git'
  args = ['cl', 'format', '--upstream=' + options.base]

  args.push('--python')
  args.push('--no-rust-fmt')

  if (options.full) args.push('--full')
  if (options.diff) args.push('--diff')

  const clFormatResult = util
    .run(cmd, args, {
      ...cmdOptions,
      stdio: 'pipe'
    })
    .stdout.toString()

  const changedFiles = util.getChangedFiles(options)
  const prettierResult = await runPrettier(changedFiles, options.diff)

  if (options.diff) {
    let result = true
    if (clFormatResult !== '') {
      console.error(clFormatResult)
      console.error('git cl format failed')
      result = false
    }

    if (prettierResult.length > 0) {
      console.error(prettierResult.join('\n'))
      console.error('Prettier check failed')
      result = false
    }
    if (!result) {
      process.exit(1)
    }
  }
}

const runPrettier = async (files, diff) => {
  const result = []
  const options = require(path.join(config.braveCoreDir, '.prettierrc'))
  for (const file of files) {
    const fileInfo = await prettier.getFileInfo(file, {
      ignorePath: path.join(config.braveCoreDir, '.prettierignore'),
      withNodeModules: false
    })

    if (fileInfo.ignored || !fileInfo.inferredParser) {
      continue
    }

    const content = fs.readFileSync(file, { encoding: 'utf-8' })
    const formatted = await prettier.format(content, {
      ...options,
      parser: fileInfo.inferredParser
    })
    if (diff) {
      if (content !== formatted) {
        result.push(`${file} is not formatted`)
      }
    } else {
      fs.writeFileSync(file, formatted)
    }
  }
  return result
}

program
  .option('--base <base branch>', 'set the destination branch for the PR')
  .option(
    '--full',
    'format all lines in changed files instead of only the changed lines'
  )
  .option('--diff', 'print diff to stdout rather than modifying files')
  .action(runFormat)

program.parse(process.argv)
