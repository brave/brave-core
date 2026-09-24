// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../lib/checkEnvironment.js'

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { isDeepStrictEqual } from 'node:util'
import { Command, Option } from 'commander'
import { parseInteger } from '../lib/commandsUtils.ts'
import config from '../lib/config.ts'
import util from '../lib/util.js'
import { isCI } from '../lib/ciDetect.ts'

const presubmitModes = ['upload', 'commit'] as const
type PresubmitMode = (typeof presubmitModes)[number]

function createPresubmitModeOption() {
  const defaultModes: PresubmitMode[] = isCI ? ['upload', 'commit'] : ['upload']

  return new Option(
    '--mode <mode...>',
    'set one or more presubmit modes; defaults to upload and commit on CI, '
      + 'upload otherwise',
  )
    .choices(presubmitModes)
    .default(defaultModes, defaultModes.join(' '))
}

const program = new Command()
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
  .addOption(
    new Option('--verbose [arg]', 'pass --verbose 2 for more debugging info')
      .preset('1')
      .argParser(parseInteger),
  )
  .addOption(createPresubmitModeOption())
  .option('--fix', 'try to fix found issues automatically')
  .option('--json <output>', 'an output file for a JSON report')
  .action(runPresubmit)

async function runPresubmit(options: ReturnType<typeof program.opts>) {
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

  if (options.all) {
    if (options.all === 'brave' || options.all === true /* default value */) {
      cmdOptions.env.PRESUBMIT_ALL_BRAVE = '1'
    } else if (options.all !== 'chromium') {
      throw new Error(`Invalid all mode: ${options.all}`)
    }
  }

  if (options.fix) {
    cmdOptions.env.PRESUBMIT_FIX = '1'
  }

  await using jsonOutput = JsonOutput.create(options.json, options.mode)

  const createClPresubmitCall = (mode: PresubmitMode) => {
    const args = ['cl', 'presubmit', options.base, '--force']

    if (mode === 'upload') {
      args.push('--upload')
    }
    if (options.all === 'chromium') {
      args.push('--all')
    }
    if (options.files) {
      args.push('--files', options.files)
    }
    if (options.verbose) {
      args.push(...Array.from({ length: options.verbose }, () => '--verbose'))
    }
    if (jsonOutput) {
      args.push('--json', jsonOutput.pathFor(mode))
    }

    return args
  }

  for (const mode of options.mode) {
    util.run('git', createClPresubmitCall(mode), cmdOptions)
  }

  jsonOutput?.mergeReports()
}

class JsonOutput implements AsyncDisposable {
  private readonly outputPath: string
  private readonly tempReportDir:
    | ReturnType<typeof fs.mkdtempDisposableSync>
    | undefined

  private constructor(
    outputPath: string,
    tempReportDir?: ReturnType<typeof fs.mkdtempDisposableSync>,
  ) {
    this.outputPath = outputPath
    this.tempReportDir = tempReportDir
  }

  static create(
    output: string | undefined,
    modes: readonly PresubmitMode[],
  ): JsonOutput | undefined {
    if (!output) {
      return undefined
    }

    const outputPath = path.resolve(config.braveCoreDir, output)
    const runsBothModes = modes.includes('upload') && modes.includes('commit')
    const tempReportDir = runsBothModes
      ? fs.mkdtempDisposableSync(path.join(os.tmpdir(), 'brave-presubmit-'))
      : undefined

    return new JsonOutput(outputPath, tempReportDir)
  }

  pathFor(mode: PresubmitMode): string {
    return this.tempReportDir
      ? path.join(this.tempReportDir.path, `${mode}.json`)
      : this.outputPath
  }

  mergeReports(): void {
    if (!this.tempReportDir) {
      return
    }

    const uploadReport = this.readReport('upload')
    const commitReport = this.readReport('commit')
    const mergedReport = { ...uploadReport }

    for (const [key, value] of Object.entries(commitReport)) {
      if (Array.isArray(value)) {
        const mergedResults: unknown[] = []
        const results = [
          ...(Array.isArray(mergedReport[key]) ? mergedReport[key] : []),
          ...value,
        ]
        for (const result of results) {
          if (!mergedResults.some((item) => isDeepStrictEqual(item, result))) {
            mergedResults.push(result)
          }
        }
        mergedReport[key] = mergedResults
      } else if (!(key in mergedReport)) {
        mergedReport[key] = value
      }
    }

    fs.writeFileSync(this.outputPath, JSON.stringify(mergedReport), 'utf8')
  }

  async [Symbol.asyncDispose](): Promise<void> {
    await this.tempReportDir?.remove()
  }

  private readReport(mode: PresubmitMode): Record<string, unknown> {
    return JSON.parse(fs.readFileSync(this.pathFor(mode), 'utf8')) as Record<
      string,
      unknown
    >
  }
}

program.parseAsync(process.argv)
