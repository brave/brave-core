// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const { promisify } = require('util')
const { readFile, writeFile } = require('fs/promises')
const exec = promisify(require('child_process').execFile)
const path = require('path')
const config = require('./config')
const { unlink } = require('fs-extra')
const { randomUUID } = require('crypto')
const { getApplicableFilters, getTestsToRun } = require('./testUtils')

const getTestTargets = (outDir, filters = ['//*']) =>
  exec(
    'gn',
    ['ls', outDir, '--type=executable', '--testonly=true', ...filters],
    { env: config.defaultOptions.env, shell: config.defaultOptions.shell },
  ).then((x) => x.stdout.trim().split('\n'))

const asGnTarget = (file) =>
  ('//brave/' + file).replace('brave/chromium_src/', '')

// set base = HEAD if you want to ignore the current workspace changes
async function getModifiedFiles(target = 'HEAD~', base = null) {
  const args = ['diff', '--name-only', target, base].filter((x) => x)

  return exec('git', args, { maxBuffer: 1024 * 1024 * 50 }).then((x) =>
    x.stdout
      .trim()
      .split('\n')
      .filter((x) => x),
  )
}

async function getReferenceCommit() {
  if (process.env.GIT_PREVIOUS_SUCCESSFUL_COMMIT) {
    return process.env.GIT_PREVIOUS_SUCCESSFUL_COMMIT
  }

  const currentBranch = await exec('git', [
    'rev-parse',
    '--abbrev-ref',
    'HEAD',
  ]).then((x) => x.stdout)

  if (currentBranch !== 'master') {
    return 'origin/master'
  }

  // compare with previous commit
  return 'HEAD~'
}

async function analyzeAffectedTests(
  outDir,
  { filters = ['//*'], files = [], since = null } = {},
) {
  // JENKINS sets GIT_PREVIOUS_SUCCESSFUL_COMMIT
  // TODO: find TeamCity equivalent.
  // TODO: we can optimize further by getting the last failure commit
  const targetCommit =
    !since || since === true ? await getReferenceCommit() : since

  const root = path.resolve(process.cwd(), '../')
  outDir = path.isAbsolute(outDir) ? outDir : `${root}/${outDir}`
  const testTargets = await getTestTargets(outDir, filters)
  const modifiedFiles = [
    ...(await getModifiedFiles(targetCommit)),
    ...files,
  ].map(asGnTarget)

  const toAnalyze = {
    files: modifiedFiles,
    test_targets: testTargets,
  }

  const uuid = randomUUID()
  // paths are relative to package.json
  await writeFile(
    `${root}/out/analyze-${uuid}.json`,
    JSON.stringify(toAnalyze, null, 2),
    'utf-8',
  )

  const { env, shell } = config.defaultOptions
  await exec(
    'gn',
    [
      'analyze',
      outDir,
      `${root}/out/analyze-${uuid}.json`,
      `${root}/out/out-${uuid}.json`,
    ],
    { env, shell },
  )

  const output = await readFile(`${root}/out/out-${uuid}.json`, 'utf-8').then(
    JSON.parse,
  )

  await Promise.all([
    unlink(`${root}/out/analyze-${uuid}.json`),
    unlink(`${root}/out/out-${uuid}.json`),
  ])

  return {
    outDir,
    filters,
    ...toAnalyze,
    targetCommit,
    affectedTests: output.test_targets,
  }
}

function createTestFilter(config, suite) {
  if (!suite) {
    return () => true
  }

  const tests = new Set(getTestsToRun(config, suite))

  return (x) => tests.has(x.split(':')[1])
}

async function getAffectedTests(args = {}) {
  const { suite } = args
  const cwd = process.cwd()

  const analysis = await analyzeAffectedTests(config.outputDir, args)
  const affectedTests = analysis.affectedTests.filter(
    createTestFilter(config, suite),
  )

  const modified = new Set(analysis.files)

  const additionalTests = analysis.test_targets
    .map((x) => x.split(':')[1])
    .flatMap((test) =>
      getApplicableFilters(config, test).map((filter) => ({ test, filter })),
    )
    .filter(({ filter }) =>
      modified.has('//brave/' + path.relative(cwd, filter)),
    )
    .map(({ test }) => test)

  return [
    ...new Set([
      ...affectedTests.map((x) => x.split(':')[1]),
      ...additionalTests,
    ]),
  ]
}

module.exports = {
  analyzeAffectedTests,
  getAffectedTests,
}
