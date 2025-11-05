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
    'Format changed code with git cl format and prettier.\n'
      + 'The changed code is the difference between the current files on disk\n'
      + 'and the base branch (origin/master by default).\n'
      + 'This script is also called as part of presubmit checks.',
  )
  .option('--base <base branch>', 'set the destination branch for the PR')
  .option(
    '--full',
    'format all lines in changed files instead of only the changed lines',
  )
  .option(
    '--presubmit',
    'filter out formatters that have dedicated presubmit checks.'
      + 'Used when running the script from a presubmit',
  )
  .option(
    '--dry-run',
    "dry run, don't format files, just check if they are formatted",
  )
  .option(
    '--only-prettier',
    'only run prettier, ignore the rest of the formatters.',
  )
  .option(
    '--all-files',
    'format all files, not just the changed files.'
      + 'Only for prettier and mojom formatters.',
  )
  .action(runFormat)

// Replace the first 4 lines of the diff output with the before/after
// format header.
const convertDiff = (diffOutput, file) => {
  let pos = -1
  for (let i = 0; i < 4; i++) {
    pos = diffOutput.indexOf('\n', pos + 1)
  }
  const diffOutputWithHeader =
    `--- ${file} (original)\n+++ ${file} (reformatted)\n`
    + diffOutput.slice(pos + 1)
  return diffOutputWithHeader
}

const getAllFiles = () => {
  return util
    .runGit(
      config.braveCoreDir,
      ['ls-tree', 'HEAD', '--name-only', '-r'],
      false,
      { encoding: 'utf-8', maxBuffer: 10 * 1024 * 1024 },
    )
    .split('\n')
}

const formatOutput = (result) => {
  return [result.stdout, result.stderr]
    .filter((v) => v.length)
    .join('\nstderr:\n')
}

// A function that formats the code in the current diff with base branch.
// It uses git cl format and prettier, then aggregates the results.
async function runFormat(options = {}) {
  if (!options.base) {
    options.base = 'origin/master'
  }
  const skipLogging = options.presubmit

  let cmdOptions = config.defaultOptions
  cmdOptions.cwd = config.braveCoreDir
  cmdOptions = util.mergeWithDefault(cmdOptions)
  const args = ['cl', 'format', '--upstream=' + options.base]

  args.push('--python')

  if (options.full) {
    args.push('--full')
  }

  if (options.presubmit) {
    args.push('--presubmit')
  }

  if (options.dryRun) {
    // `--dry-run` alone only generates a non-zero exit code on format issues.
    // `--diff` is required to get the actual change in stdout.
    args.push('--dry-run', '--diff')
  }

  let formatIssues = []

  const shouldRunGitClFormat = !options.onlyPrettier
  const shouldRunPrettier = true
  const shouldRunMojomFormat = !options.onlyPrettier

  if (shouldRunGitClFormat) {
    const clFormatResult = util.run('git', [...args], {
      ...cmdOptions,
      continueOnFail: true,
      skipLogging,
      stdio: 'pipe',
    })

    const clFormatOutput = formatOutput(clFormatResult)

    if (clFormatResult.status === 2 && options.dryRun) {
      formatIssues.push(clFormatOutput)
    } else if (clFormatResult.status !== 0) {
      Log.error('Fatal: git cl format failed:\n' + clFormatOutput)
      process.exit(clFormatResult.status)
    }
  }

  const filesToFormat = options.allFiles
    ? getAllFiles()
    : util.getChangedFiles(config.braveCoreDir, options.base, skipLogging)

  if (shouldRunPrettier) {
    formatIssues.push(...(await runPrettier(filesToFormat, options.dryRun)))
  }
  if (shouldRunMojomFormat) {
    formatIssues.push(...(await runMojomFormat(filesToFormat, options.dryRun)))
  }

  if (options.dryRun && formatIssues.length > 0) {
    console.error(formatIssues.join('\n'))
    process.exit(2)
  }
}

const handleDifference = async (file, dryRun, formatted) => {
  if (!dryRun) {
    await fs.writeFile(file, formatted)
    return
  }

  // Use `echo <formatted> | git diff --no-index <file> -` to show diff
  const diffResult = spawnSync('git', ['diff', '--no-index', file, '-'], {
    stdio: 'pipe',
    input: formatted,
  })
  if (diffResult.status === 1) {
    // Differences were found
    return convertDiff(diffResult.stdout.toString(), file)
  } else {
    return `Can't show diff for ${file}:\n ${formatOutput(diffResult)}`
  }
}

const runPrettierForFile = async (file, dryRun, options, ignorePath) => {
  const fileInfo = await prettier.getFileInfo(file, {
    ignorePath: ignorePath,
    withNodeModules: false,
  })

  if (fileInfo.ignored || !fileInfo.inferredParser) {
    return
  }

  const content = await fs.readFile(file, { encoding: 'utf-8' })
  const formatted = await prettier.format(content, {
    ...options,
    filepath: file,
    parser: fileInfo.inferredParser,
  })
  if (content !== formatted) {
    return await handleDifference(file, dryRun, formatted)
  }
}

const runPrettier = async (files, dryRun) => {
  console.log('run prettier for', files.length, 'files')
  const options = require(path.join(config.braveCoreDir, '.prettierrc'))
  const ignorePath = path.join(config.braveCoreDir, '.prettierignore')
  if (!fs.existsSync(ignorePath)) {
    throw new RuntimeError(`${ignorePath} file not found`)
  }

  const prettierIssues = []
  for (const file of files) {
    try {
      const issue = await runPrettierForFile(file, dryRun, options, ignorePath)
      if (issue) {
        prettierIssues.push(issue)
      }
    } catch (e) {
      prettierIssues.push(`Can't format ${file}:\n ${e}`)
    }
  }

  return prettierIssues
}

const runMojomFormatForFile = async (file, dryRun) => {
  if (!file.endsWith('.mojom')) {
    return
  }
  // Mojom formatting is experimental. Only these files are formatted by now.
  const mojomFormatAllowList = ['**/brave_wallet/**/*.mojom']

  if (
    !mojomFormatAllowList.some((pattern) => path.matchesGlob(file, pattern))
  ) {
    return
  }

  const content = await fs.readFile(file, { encoding: 'utf-8' })
  if (!content) {
    return
  }

  const mojomFormatArgs = [
    path.join(
      config.srcDir,
      'mojo',
      'public',
      'tools',
      'mojom',
      'mojom_format.py',
    ),
  ]
  const formatResult = util.run('python3', mojomFormatArgs, { input: content })
  const formatted = formatResult.stdout.toString().split(/\r?\n/).join('\n')
  if (!formatted) {
    return `Can't format ${file}:\n`
  }

  if (content !== formatted) {
    return await handleDifference(file, dryRun, formatted)
  }
}

const runMojomFormat = async (files, dryRun) => {
  const mojomFormatIssues = []
  for (const file of files) {
    try {
      const issue = await runMojomFormatForFile(file, dryRun)
      if (issue) {
        mojomFormatIssues.push(issue)
      }
    } catch (e) {
      mojomFormatIssues.push(`Can't format ${file}:\n ${e}`)
    }
  }

  return mojomFormatIssues
}

program.parse(process.argv)
