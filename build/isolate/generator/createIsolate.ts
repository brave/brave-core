// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Creates test isolates: archives with gtest-based test binaries, their
// runtime dependencies and a runner (build/isolate/runner) to run them on
// another host. See build/isolate/README.md.

import assert from 'node:assert'
import { spawnSync } from 'node:child_process'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import type { IsolateManifest } from '../runner/manifest.ts'
import type { TestOptions } from '../../commands/lib/buildOptions.ts'
import type { Config } from '../../commands/lib/config.ts'
import * as Log from '../../commands/lib/log.ts'
import {
  getTestRunArgs,
  getTestRunEnvDefaults,
} from '../../commands/lib/testRunSpec.ts'
import {
  getApplicableFilters,
  getTestBinary,
  getTestsToRun,
} from '../../commands/lib/testUtils.ts'

// Isolate layout, as expected by build/isolate/runner/run.ts.
const RUNNER_DIR_NAME = 'runner'
const MANIFEST_FILE_NAME = 'manifest.json'
const SRC_DIR_NAME = 'src'

// Files of build/isolate/runner to copy into the isolate runner directory.
const RUNNER_FILES = [
  'check_environment.cjs',
  'manifest.ts',
  'package.json',
  'run.ts',
]

// Returns the entrypoint of build/isolate to copy into the isolate root.
function getEntrypointFile(config: Config) {
  return config.targetOS === 'win' ? 'run.bat' : 'run.sh'
}

// Returns the tests to isolate for `suites`, in order and without duplicates.
// Test groups that write <out_dir>/<group>.json (see brave/BUILD.gn) are
// expanded into their tests.
export function resolveIsolateTests(config: Config, suites: string[]) {
  return [...new Set(suites.flatMap((suite) => getTestsToRun(config, suite)))]
}

function getOutputDirRelativeToSrc(config: Config) {
  const outputDir = path.relative(config.srcDir, config.outputDir)
  if (outputDir.startsWith('..') || path.isAbsolute(outputDir)) {
    throw new Error(
      `The output directory ${config.outputDir} must be inside ${config.srcDir}`,
    )
  }
  return outputDir
}

export function createIsolateManifest(
  config: Config,
  tests: string[],
  options: TestOptions,
  passthroughArgs: string[],
  isTeamcity?: boolean,
): IsolateManifest {
  const cwd = path.posix.join(
    options.isolate_data_dir,
    SRC_DIR_NAME,
    getOutputDirRelativeToSrc(config),
  )
  return {
    suites: tests.map((suite) => ({
      name: suite,
      cwd,
      binary: path.relative(config.outputDir, getTestBinary(config, suite)),
      args: getTestRunArgs(config, suite, options, passthroughArgs, {
        pathStyle: 'relative',
        ...(isTeamcity === undefined ? {} : { isTeamcity }),
      }),
      envDefaults: getTestRunEnvDefaults(config, suite, {
        pathStyle: 'relative',
      }),
    })),
  }
}

// Returns the files the isolate needs, relative to the source directory: the
// runtime deps of each test, which gn writes to <out_dir>/<test>.runtime_deps
// (see testing/test.gni), and its filter files.
export function collectIsolateFiles(config: Config, tests: string[]): string[] {
  const files = new Set<string>()
  const addFile = (absolutePath: string) => {
    const file = path.relative(config.srcDir, absolutePath)
    if (file === '' || file.startsWith('..') || path.isAbsolute(file)) {
      throw new Error(`${absolutePath} is outside of ${config.srcDir}`)
    }
    files.add(file)
  }

  for (const suite of tests) {
    const runtimeDepsFile = path.join(config.outputDir, `${suite}.runtime_deps`)
    if (!fs.existsSync(runtimeDepsFile)) {
      throw new Error(
        `${runtimeDepsFile} doesn't exist: ${suite} isn't a test target, or `
          + 'a test group without <out_dir>/<group>.json',
      )
    }
    for (const line of fs.readFileSync(runtimeDepsFile, 'utf8').split('\n')) {
      if (line.trim() !== '') {
        addFile(path.resolve(config.outputDir, line.trim()))
      }
    }
    getApplicableFilters(config, suite).forEach(addFile)
  }

  addFile(path.join(config.braveCoreDir, 'build', '.ci_features'))

  if (config.isLsan()) {
    addFile(
      path.join(
        config.braveCoreDir,
        'test',
        'sanitizers',
        'lsan_suppressions.cfg',
      ),
    )
  }

  return [...files].sort()
}

// Writes the isolate archive (tar.xz) to `outputFile`.
export function writeIsolateArchive(
  config: Config,
  manifest: IsolateManifest,
  files: string[],
  outputFile: string,
  dataDirName: string,
  isolateDir: string = path.join(config.braveCoreDir, 'build', 'isolate'),
) {
  Log.status(`Creating ${outputFile}`)
  const stagingDir = fs.mkdtempSync(path.join(os.tmpdir(), 'brave-isolate-'))
  try {
    const stagingRunnerDir = path.join(stagingDir, RUNNER_DIR_NAME)
    fs.mkdirSync(stagingRunnerDir)
    const entrypointFile = getEntrypointFile(config)
    fs.copyFileSync(
      path.join(isolateDir, entrypointFile),
      path.join(stagingDir, entrypointFile),
    )
    for (const file of RUNNER_FILES) {
      fs.copyFileSync(
        path.join(isolateDir, RUNNER_DIR_NAME, file),
        path.join(stagingRunnerDir, file),
      )
    }
    fs.writeFileSync(
      path.join(stagingDir, MANIFEST_FILE_NAME),
      JSON.stringify(manifest, null, 2) + '\n',
    )

    // Archive the source files through a symlink, so they land in the data
    // directory without being copied.
    const dataDir = path.join(stagingDir, dataDirName)
    fs.mkdirSync(dataDir)
    fs.symlinkSync(config.srcDir, path.join(dataDir, SRC_DIR_NAME))

    const fileList = path.join(stagingDir, 'files.txt')
    fs.writeFileSync(
      fileList,
      files
        .map((file) => path.posix.join(dataDirName, SRC_DIR_NAME, file))
        .join('\n') + '\n',
    )

    const output = path.resolve(outputFile)
    fs.mkdirSync(path.dirname(output), { recursive: true })
    const result = spawnSync(
      'tar',
      [
        '-C',
        stagingDir,
        '-c',
        '-I',
        'pixz',
        // Prints the archived size about every 100 MiB.
        '--checkpoint=10000',
        '--checkpoint-action=echo=%T',
        '--totals',
        '--hard-dereference',
        // Skips files that weren't built, with a warning.
        '--ignore-failed-read',
        '-f',
        output,
        entrypointFile,
        MANIFEST_FILE_NAME,
        RUNNER_DIR_NAME,
        '--verbatim-files-from',
        '-T',
        fileList,
      ],
      { stdio: 'inherit' },
    )
    if (result.error) {
      throw result.error
    }
    if (result.status !== 0) {
      throw new Error(`tar failed with exit code ${result.status}`)
    }
    Log.status(`Created ${outputFile}`)
  } finally {
    fs.rmSync(stagingDir, { recursive: true, force: true })
  }
}

// Exits with an error if an isolate can't be created with `options`. Called
// before the build, so that it doesn't run in vain.
export function checkIsolateOptions(config: Config, options: TestOptions) {
  if (process.platform !== 'linux') {
    Log.error('Creating test isolates is only supported on Linux')
    process.exit(1)
  }
  if (config.isMobile()) {
    Log.error(`Test isolates aren't supported for ${config.targetOS}`)
    process.exit(1)
  }
  const unsupportedOptions = {
    '--base': options.base,
    '--output_xml': options.output_xml,
    '--skip_run': options.skip_run,
  }
  for (const [name, value] of Object.entries(unsupportedOptions)) {
    if (value) {
      Log.error(`${name} is not supported with --isolate`)
      process.exit(1)
    }
  }
  const dataDirName = options.isolate_data_dir
  if (
    !/^[\w.-]+$/.test(dataDirName)
    || dataDirName === '.'
    || dataDirName === '..'
    || dataDirName === RUNNER_DIR_NAME
  ) {
    Log.error(`Invalid --isolate_data_dir: ${dataDirName}`)
    process.exit(1)
  }
}

// Creates the isolate archive `options.isolate` for the built `suites`.
export function createIsolate(
  config: Config,
  suites: string[],
  options: TestOptions,
  passthroughArgs: string[],
) {
  const outputFile = options.isolate
  assert(outputFile)
  // Test groups are expanded again after the build, which writes the
  // <out_dir>/<group>.json files of new output directories.
  const tests = resolveIsolateTests(config, suites)
  Log.status(`Isolating ${tests.length} tests: ${tests.join(', ')}`)
  writeIsolateArchive(
    config,
    createIsolateManifest(config, tests, options, passthroughArgs),
    collectIsolateFiles(config, tests),
    outputFile,
    options.isolate_data_dir,
  )
}
