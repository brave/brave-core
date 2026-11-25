// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { spawnSync } from 'node:child_process'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import type { TestOptions } from '../../commands/lib/buildOptions.ts'
import type { Config } from '../../commands/lib/config.ts'
import {
  collectIsolateFiles,
  createIsolateManifest,
  resolveIsolateTests,
  writeIsolateArchive,
} from './createIsolate.ts'

let tmpDir: string
let srcDir: string
let outputDir: string

function makeConfig() {
  return {
    srcDir,
    outputDir,
    // Tests run from the brave directory.
    braveCoreDir: srcDir,
    targetOS: 'linux',
    targetArch: 'x64',
    isIOS: () => false,
    isAndroid: () => false,
    isMobile: () => false,
    isAsan: () => false,
    isLsan: () => false,
  } as unknown as Config
}

const options = {
  v: 0,
  test_launcher_jobs: 4,
  isolate_data_dir: 'data',
} as TestOptions

function writeFile(file: string, content = '', mode = 0o644) {
  const filePath = path.join(srcDir, file)
  fs.mkdirSync(path.dirname(filePath), { recursive: true })
  fs.writeFileSync(filePath, content, { mode })
  return filePath
}

beforeEach(() => {
  tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'create-isolate-'))
  srcDir = path.join(tmpDir, 'src')
  outputDir = path.join(srcDir, 'out', 'Release')
  fs.mkdirSync(outputDir, { recursive: true })
  writeFile('build/.ci_features')
})

afterEach(() => {
  fs.rmSync(tmpDir, { recursive: true, force: true })
})

describe('resolveIsolateTests', () => {
  it('expands test groups and drops duplicates', () => {
    writeFile(
      'out/Release/all_tests.json',
      JSON.stringify(['//a:a_unittests', '//b:browser_tests']),
    )
    expect(
      resolveIsolateTests(makeConfig(), ['all_tests', 'browser_tests', 'c']),
    ).toEqual(['a_unittests', 'browser_tests', 'c'])
  })
})

describe('createIsolateManifest', () => {
  it('describes how to run each test relative to the manifest', () => {
    writeFile('test/filters/browser_tests.filter')
    expect(
      createIsolateManifest(
        makeConfig(),
        ['browser_tests'],
        options,
        ['--y'],
        false,
      ),
    ).toEqual({
      suites: [
        {
          name: 'browser_tests',
          cwd: 'data/src/out/Release',
          binary: 'browser_tests',
          args: [
            '--enable-logging=stderr',
            '--v=0',
            '--test-launcher-jobs=4',
            '--y',
            '--test-launcher-filter-file=../../test/filters/browser_tests.filter',
          ],
          envDefaults: {
            LLVM_PROFILE_FILE: 'coverage/browser_tests/%4m.profraw',
          },
        },
      ],
    })
  })
})

describe('collectIsolateFiles', () => {
  it('collects runtime deps and filters relative to the source dir', () => {
    writeFile('test/filters/a_tests.filter')
    writeFile('out/Release/a_tests.runtime_deps', './a_tests\n../../data/\n')
    writeFile('out/Release/b_tests.runtime_deps', './b_tests\n../../data/\n')
    expect(collectIsolateFiles(makeConfig(), ['a_tests', 'b_tests'])).toEqual([
      'build/.ci_features',
      'data',
      'out/Release/a_tests',
      'out/Release/b_tests',
      'test/filters/a_tests.filter',
    ])
  })

  it('rejects files outside of the source dir', () => {
    writeFile('out/Release/a_tests.runtime_deps', '../../../outside\n')
    expect(() => collectIsolateFiles(makeConfig(), ['a_tests'])).toThrow(
      /is outside of/,
    )
  })

  it('rejects targets without runtime deps', () => {
    expect(() => collectIsolateFiles(makeConfig(), ['a_group'])).toThrow(
      /a_group isn't a test target/,
    )
  })
})

const describeOnLinux = process.platform === 'linux' ? describe : describe.skip

describeOnLinux('writeIsolateArchive', () => {
  it('creates an archive that runs its tests', () => {
    writeFile(
      'out/Release/fake_tests',
      '#!/bin/sh\necho "$@" > args.txt\nexit "${FAKE_STATUS:-0}"\n',
      0o755,
    )
    writeFile('out/Release/fake_data/data.txt', 'data')
    writeFile('test/filters/fake_tests.filter')
    writeFile(
      'out/Release/fake_tests.runtime_deps',
      './fake_tests\n./fake_data/\n./not_built\n',
    )

    const config = makeConfig()
    const tests = ['fake_tests']
    const archive = path.join(tmpDir, 'isolate', 'tests.tar.xz')
    writeIsolateArchive(
      config,
      createIsolateManifest(config, tests, options, [], false),
      collectIsolateFiles(config, tests),
      archive,
      'data',
      path.resolve('build', 'isolate'),
    )

    const extractDir = path.join(tmpDir, 'extract')
    fs.mkdirSync(extractDir)
    expect(spawnSync('tar', ['-xJf', archive, '-C', extractDir]).status).toBe(0)
    const listing = spawnSync('tar', ['-tJf', archive], { encoding: 'utf8' })
      .stdout.split('\n')
      .filter((line) => line !== '' && !line.endsWith('/'))
      .sort()
    expect(listing).toEqual([
      'data/src/build/.ci_features',
      'data/src/out/Release/fake_data/data.txt',
      'data/src/out/Release/fake_tests',
      'data/src/test/filters/fake_tests.filter',
      'manifest.json',
      'run.sh',
      'runner/check_environment.cjs',
      'runner/manifest.ts',
      'runner/package.json',
      'runner/run.ts',
    ])

    const runSh = path.join(extractDir, 'run.sh')
    expect(fs.statSync(runSh).mode & 0o111).not.toBe(0)
    const run = (env: NodeJS.ProcessEnv = {}) =>
      spawnSync(runSh, ['--extra'], {
        encoding: 'utf8',
        env: { ...process.env, ...env },
      })

    expect(run().status).toBe(0)
    expect(
      fs.readFileSync(
        path.join(extractDir, 'data/src/out/Release/args.txt'),
        'utf8',
      ),
    ).toBe(
      '--enable-logging=stderr --v=0 --test-launcher-jobs=4 '
        + '--test-launcher-filter-file=../../test/filters/fake_tests.filter '
        + '--extra\n',
    )
    expect(run({ FAKE_STATUS: '3' }).status).toBe(3)
  })

  it('uses run.bat for windows and the given data dir name', () => {
    writeFile('out/Release/fake_tests.exe')
    writeFile('out/Release/fake_tests.runtime_deps', './fake_tests.exe\n')

    const config = { ...makeConfig(), targetOS: 'win' } as unknown as Config
    const tests = ['fake_tests']
    const archive = path.join(tmpDir, 'tests.tar.xz')
    writeIsolateArchive(
      config,
      createIsolateManifest(
        config,
        tests,
        { ...options, isolate_data_dir: 'test_data' },
        [],
        false,
      ),
      collectIsolateFiles(config, tests),
      archive,
      'test_data',
      path.resolve('build', 'isolate'),
    )

    const listing = spawnSync('tar', ['-tJf', archive], { encoding: 'utf8' })
      .stdout.split('\n')
      .filter((line) => line !== '' && !line.endsWith('/'))
      .sort()
    expect(listing).toEqual([
      'manifest.json',
      'run.bat',
      'runner/check_environment.cjs',
      'runner/manifest.ts',
      'runner/package.json',
      'runner/run.ts',
      'test_data/src/build/.ci_features',
      'test_data/src/out/Release/fake_tests.exe',
    ])
    const manifest = JSON.parse(
      spawnSync('tar', ['-xJOf', archive, 'manifest.json'], {
        encoding: 'utf8',
      }).stdout,
    )
    expect(manifest.suites[0].cwd).toBe('test_data/src/out/Release')
  })
})
